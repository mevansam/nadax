//
//  HttpService.m
//  Framework
//
//  Created by Mevan Samaratunga on 5/27/11.
//  Copyright 2011 Fidelity Investments. All rights reserved.
//

#import "iHttpService.h"
#import "Framework.h"
#import "MessageBus.h"
#import "AppConfig.h"

#include <string>
#include <list>

#include "MessageBusManager.h"
#include "HttpService.h"
#include "DynaModel.h"
#include "number.h"
#include "log.h"

#define HANDLE_COOKIE_HEADER_LIMITATION
#define MAX_SUPPORTED_COOKIE_HEADER_SIZE  4099

#define TRUSTED_DOMAINS  @"fidelity.com", @"fmr.com"

NSString* const HTTP_METHOD_GET  = @"GET";
NSString* const HTTP_METHOD_POST = @"POST";

NSString* const HTTP_HEADER_USER_AGENT   = @"User-Agent";
NSString* const HTTP_HEADER_CONTENT_TYPE = @"Content-Type";
NSString* const HTTP_HEADER_ACCEPT       = @"Accept";

NSString* const HTTP_CONTENT_XML            = @"text/xml";
NSString* const HTTP_CONTENT_JSON           = @"application/json";
NSString* const HTTP_CONTENT_MULTI_PART     = @"multipart/form-data";
NSString* const HTTP_CONTENT_DEFAULT        = @"application/x-www-form-urlencoded";
NSString* const HTTP_CONTENT_DEFAULT_ACCEPT = @"text/xml; text/plain; text/html;";

using namespace mb;
using namespace binding;


#pragma mark -
#pragma mark Static reference data

static NSArray* _trustedDomains = nil;


#pragma mark -
#pragma mark Concurrent NSOperation and NSURLConnection delegate handler

static NSOperationQueue* _httpOperationQ = [[NSOperationQueue alloc] init];


@interface HttpServiceOperation :  NSOperation  {
    
@private
    
    NSURLConnection* urlConnection;
    
    MessagePtr* reqMessage;
    MessagePtr* respMessage;
    MessagePtr* endMessage;
    
    int execTimeout;
    
    BOOL executing;
    BOOL finished;
    
    int counter;    
    BOOL respComplete;
}

@property (nonatomic, retain) NSURLConnection* urlConnection;

@end


#pragma mark -
#pragma mark Concurrent NSOperation and NSURLConnection delegate handler implementation

@implementation HttpServiceOperation

@synthesize urlConnection;

- (id) initWithMessages:(MessagePtr)request response:(MessagePtr)response completion:(MessagePtr)completion timeout:(int)timeout {
    
    self = [super init];
    if (!self) return nil;
    
    reqMessage = new MessagePtr();
    respMessage = new MessagePtr();
    endMessage = new MessagePtr();
    
    *reqMessage = request;
    *respMessage = response;
    *endMessage = completion;
    
    execTimeout = timeout;
    
    executing = NO;
    finished = NO;
    
    counter = 0;
    respComplete = NO;
    
    return self;
}

- (void) dealloc {
    
    delete reqMessage;
    delete respMessage;
    delete endMessage;
    
    [urlConnection release];
    [super dealloc];
}

- (BOOL) isConcurrent {
    return YES;
}

- (BOOL) isExecuting {
    return executing;
}

- (BOOL) isFinished {
    return finished;
}

- (void) start {
    
    if ([self isCancelled])
    {
        [self willChangeValueForKey:@"isFinished"];
        finished = YES;
        [self didChangeValueForKey:@"isFinished"];
        return;
    }
    
    [self willChangeValueForKey:@"isExecuting"];
    [NSThread detachNewThreadSelector:@selector(main) toTarget:self withObject:nil];
    executing = YES;
    [self didChangeValueForKey:@"isExecuting"];
}

- (void) main {
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    @try {
        
        NSRunLoop* runLoop = [NSRunLoop currentRunLoop];
        [urlConnection scheduleInRunLoop:runLoop forMode:NSDefaultRunLoopMode];
        [urlConnection start];
        
        NSDate* timeoutAt = [[NSDate alloc] initWithTimeIntervalSinceNow:execTimeout];
        NSDate* now = nil;
        BOOL notTimedOut = NO;
        
        do {
            [runLoop runMode:NSDefaultRunLoopMode beforeDate:timeoutAt];
            
            now = [[NSDate alloc] init];
            notTimedOut = ([now compare:timeoutAt] == NSOrderedAscending);
            [now release];
            
        } while (!respComplete && notTimedOut);
        
        [timeoutAt release];
        
        @synchronized (self) {
            
            if (!respComplete) {
                
                ERROR( "The HTTP response for service '%s' did not complete within '%d' seconds.", 
                    (*respMessage)->getSubject().c_str(), execTimeout );
                
                NSString* errMsg = [[NSString alloc] initWithFormat:@"Request timed out after %d seconds.", execTimeout];
                (*respMessage)->setError(Message::ERR_EXECUTION_TIMEOUT, 2, [errMsg UTF8String]);
                [errMsg release];
                
                SEND_DATA((*respMessage), NULL, 0);
                respComplete = true;
            }
        }
    }
    @catch(...) {
        ERROR("Unexpected exception while in iHttpService main loop");
    }
    @finally {
        [pool release];
    }
    
    [self willChangeValueForKey:@"isFinished"];
    [self willChangeValueForKey:@"isExecuting"];
    finished = YES;
    executing = NO;
    [self didChangeValueForKey:@"isFinished"];
    [self didChangeValueForKey:@"isExecuting"];
}

- (BOOL) connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
    return [protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust];
}

- (void) connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
    
    if ([challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
        
        NSString* domain = challenge.protectionSpace.host;
        NSRange dot = [domain rangeOfString:@"."];
        if(dot.location != NSNotFound && ((dot.location + 1) < [domain length]))
            domain = [domain substringFromIndex:(dot.location + 1)];
        
        if ([_trustedDomains containsObject:domain]) {
            
            [challenge.sender 
                useCredential:[NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust] 
                forAuthenticationChallenge:challenge];
        }
    }
    
    [challenge.sender continueWithoutCredentialForAuthenticationChallenge:challenge];
}

- (void) connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    NSHTTPURLResponse* httpResponse = (NSHTTPURLResponse *) response;
    NSInteger statusCode = [httpResponse statusCode];
    
    NSString* contentType = [[httpResponse allHeaderFields] valueForKey:HTTP_HEADER_CONTENT_TYPE];
    if ([contentType hasPrefix:HTTP_CONTENT_XML])
        (*respMessage)->setContentType(Message::CNT_XML);
    else if ([contentType hasPrefix:HTTP_CONTENT_JSON])
        (*respMessage)->setContentType(Message::CNT_JSON);
    
    NSDictionary* headers = [httpResponse allHeaderFields];
    for (NSString* name in [headers allKeys]) {
        (*respMessage)->getMetaData()[[name UTF8String]] = [[headers valueForKey:name] UTF8String];
    }
    
    if (statusCode != 200) {
        
        NSString* errMsg = [[NSString alloc] initWithFormat:@"Service call returned HTTP status: %d", statusCode];
        (*respMessage)->setError(Message::ERR_SERVICE, statusCode, [errMsg UTF8String]);
        [errMsg release];
    }
}

- (NSURLRequest *) connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse {
    
    if (redirectResponse) {
        
        @synchronized (self) {
            
            if (!respComplete) {
                
                NSURL* responseUrl = [request URL];

                NSString* errMsg = [[NSString alloc] initWithFormat:@"Ignoring response being redirected to '%@'.", responseUrl];
                (*respMessage)->setError(Message::ERR_SERVICE, 302, [errMsg UTF8String]);
                
                NSLog( @"Service reqeest for '%s' was redirected to '%s' so it will be considered an error.", 
                    (*reqMessage)->getSubject().c_str(), [[responseUrl description] UTF8String] );
                
                [errMsg release];

                SEND_DATA((*respMessage), NULL, 0);
                respComplete = YES;
            }
        }
        
        [connection cancel];
        return nil;
    }
    
    return request;
}

- (void) connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    
    @synchronized (self) {
        
        if (!respComplete) {
            
            char* buffer = (char *) [data bytes];
            int len = [data length];
            
            if (!(counter++)) {
                
                // Trim leading spaces as it causes XML parsing errors
                
                while (len > 0 && *buffer == ' ') {
                    --len;
                    ++buffer;
                }
            }
            
#ifdef LOG_LEVEL_TRACE
            char* temp = (char *) calloc(len + 1, sizeof(char));
            memcpy(temp, buffer, sizeof(char) * (len + 1));
            *(temp + len) = 0;
            
            NSLog(@"Received response data: %s", temp);

            free(temp);
#endif
            
            if (!SEND_DATA((*respMessage), buffer, len)) {
                
                respComplete = YES;
                [connection cancel];
            }
        }
    }
}

- (void) connectionDidFinishLoading:(NSURLConnection *)connection {
    
    @synchronized (self) {
        
        if (!respComplete) {
            
            SEND_DATA((*respMessage), NULL, 0);
            respComplete = YES;
            
            if (endMessage->get())
                MessageBusManager::instance()->postMessage(*endMessage);
        }
    }
}

- (void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
    
    @synchronized (self) {
        
        if (!respComplete) {
            
            // Ensure that if the request was a subscription it does 
            // not repost and it cancels any subsequent subscriptions.
            ((P2PMessage *) reqMessage->get())->setControlAction(P2PMessage::CANCEL);
            
            NSString* errMsg = [[NSString alloc] initWithFormat:@"Connection failed with error: %@", error];
            (*respMessage)->setError(Message::ERR_CONNECTION_ERROR, 2, [errMsg UTF8String]);
            [errMsg release];
            
            SEND_DATA((*respMessage), NULL, 0);
            respComplete = YES;
        }
    }
}

@end


#pragma mark -
#pragma mark Framework iOS HttpService implementation

class IOSHttpServiceSettingsHandler : public Listener {
    
public:
    
    void onMessage(MessagePtr message);
    
    HttpServiceDelegate* m_delegate;
};

class IOSHttpService : public http::HttpService {

public:
    
    IOSHttpService() :
        m_streamDoNotSnap(CSTR_TRUE), 
        m_subscribeAndSnap(false), 
        m_subscriptionEnabled(false) {
    }
    
    const char* getSubject() {
        return m_subject.c_str();
    }
    
    const char* getTemplate() {
        return m_template.c_str();
    }
    
    Message* createMessage();
    void addEnvVars(std::list<Message::NameValue>& envVars);
    void execute(MessagePtr message, std::string& request);
    
    std::string m_subject;
    
    std::string m_url;
    int m_timeout;
    
    http::HttpMessage::HttpMethod m_method;
    Message::ContentType m_contentType;
    
    std::string m_template;
    
    std::list<Message::NameValue> m_headers;
    
    std::string m_streamSubject;
    std::string m_streamKey;
    std::string m_streamDoNotSnap;
    bool m_subscribeAndSnap;
    
    Number<bool> m_subscriptionEnabled;
    
    HttpServiceDelegate* m_delegate;
};


#pragma mark -
#pragma mark C struct for delegates between ObjC/C++

struct HttpServiceDelegate {
    
    IOSHttpServiceSettingsHandler* csettingsdelegate;
    
    IOSHttpService* cdelegate;
    HttpService* idelegate;
    
    boost::shared_ptr<DynaModelBindingConfig> config;
};


#pragma mark -
#pragma mark IOSHttpService Implementation

void IOSHttpServiceSettingsHandler::onMessage(MessagePtr message) {
    
    NameValueMap& data = *((NameValueMap *) message->getData());
    
    if (data.find(SUBSCRIPTION_SETTING_KEY) != data.end()) {
        
        this->m_delegate->cdelegate->m_subscriptionEnabled = (atoi(data[SUBSCRIPTION_SETTING_KEY].c_str()) != 0);
        
        TRACE("HTTP service '%s' subscription setting changed: %s", 
            this->m_delegate->cdelegate->m_subject.c_str(), 
            (this->m_delegate->cdelegate->m_subscriptionEnabled ? "enabled" : "disabled" ) );
    }
}

Message* IOSHttpService::createMessage() {
    
    Message* message = new http::HttpMessage(m_method);
    Service::initMessage(message, Message::MSG_P2P, m_contentType);
    
    if (m_delegate->config)
        message->setDataBinder(DataBinderPtr(new DynaModelBinder(m_delegate->config.get())));
    
    return message;
}

void IOSHttpService::addEnvVars(std::list<Message::NameValue>& envVars) {

    NSDictionary* settings = [AppConfig instance].settings;

    for (NSString *key in [settings allKeys]) {
        
        envVars.push_back(Message::NameValue([key UTF8String], [[settings valueForKey:key] UTF8String]));
    }
}

void IOSHttpService::execute(MessagePtr message, std::string& request) {
    
    MessageBusManager* manager = MessageBusManager::instance();
    http::HttpMessage* httpMessage = (http::HttpMessage *) message.get();
    
    P2PMessage::ControlAction controlAction = httpMessage->getControlAction();
    
    bool isControlNone = (controlAction == P2PMessage::NONE);
    bool isSubscription = (httpMessage->getType() == Message::MSG_P2P_SUB);
    bool isFirstPost = (httpMessage->getPostCount() == 0);
    
    if (!m_subscriptionEnabled && !isFirstPost)
        return;
    
    const char* respSubject = (httpMessage->getRespSubject().length() > 0 ? httpMessage->getRespSubject().c_str() : NULL);

    MessagePtr completion;
    
    if (isSubscription && m_streamSubject.length() > 0) {
        
        // Send a subscription call to the streaming service

        MessagePtr subRequest = manager->createMessage(m_streamSubject.c_str());
        NameValueMap& metaData = subRequest->getMetaData();
        metaData[DO_NOT_SNAP] = m_streamDoNotSnap;
        
        if (respSubject)
            subRequest->setRespSubject(respSubject);
        else
            subRequest->setRespSubject(message->getSubject().c_str());
        
        TRACE("Initiating subscription on service '%s'.", subRequest->getSubject().c_str());

        if (m_streamKey.length() > 0) {
            
            std::list<Message::NameValue> params = httpMessage->getParams();
            std::list<Message::NameValue>::iterator param;
            
            for (param = params.begin(); param != params.end(); param++) {
                
                if (m_streamKey == param->name) {
                    
                    TRACE("The subscription key data is: %s", param->value.c_str());
                    
                    subRequest->setData(param->name.c_str(), (void *) param->value.c_str());
                    break;
                }
            }
        }
        
        ((P2PMessage *) subRequest.get())->setControlAction(controlAction);
        
        if (!isFirstPost || !isControlNone || m_subscribeAndSnap) {
            
            MessagePtr subResponse = manager->sendMessage(subRequest);
            if (!subResponse->getError()) {
                
                Service::initMessage(
                    message.get(), 
                    subResponse.get(), 
                    subResponse->getType(), 
                    subResponse->getContentType(), 
                    respSubject );

                if (m_subscribeAndSnap) {
                    
                    // If subscription just activated then continue and snap, else
                    // return as subscription will stream subsequent updates.
                    if (subResponse->getMetaData()[SUBSCRIPTION_RESULT_CODE] == SUBSCRIPTION_RESULT_ACTIVE) {
                        
                        POST_RESPONSE(subResponse, message);
                        return;
                    }
                    
                } else {
                    
                    TRACE( "Streaming subscription is active. Continuing without polling for service '%s'", 
                        subRequest->getSubject().c_str() );
                    
                    POST_RESPONSE(subResponse, message);
                    return;
                }
            }
            
        } else {
            
            // If this is the first message and not a control message 
            // and "subscribe asap" is false, then the subcription
            // request to the streaming service should be sent
            // after the snap request/response is completed. So
            // the message is passed on to the http response handler
            // which posts this message once the http response
            // is complete.
            
            completion = subRequest;
        }
    }
    
    // Only messages that do not have a control action associated 
    // with them should continue to be processed as a request response,
    // as control actions are subscription specific.
    if (!isControlNone)        
        return;
    
    MessagePtr response(new StreamMessage());
    Service::initMessage(
        message.get(), 
        response.get(), 
        Message::MSG_RESP_STREAM, 
        Message::CNT_UNKNOWN, 
        respSubject );
    
    NameValueMap& metaData = response->getMetaData();
    
    if (m_delegate->config.get() && message->hasBinder() == 1) {
        
        metaData[DATA_IS_DYNA_MODEL] = "true";
        
        // Sets up clean up of data when message is deleted
        Datum<DynaModel> datum(response);
        
    } else
        metaData[DATA_IS_DYNA_MODEL] = "false";
    
    metaData[REQUEST_ID] = httpMessage->getId();
    
    if (isSubscription && isFirstPost) {
        // Return a subscription ID only with the first
        // response message. The receiver needs to keep
        // track of this so they can control subscriptions.
        metaData[SUBSCRIPTION_ID] = httpMessage->getId();
    }
    
    if (POST_RESPONSE(response, message) > 0) {
        
        NSMutableURLRequest* urlRequest = [[NSMutableURLRequest alloc] init];
        [urlRequest setTimeoutInterval:m_timeout];
        
        [urlRequest setValue:[MessageBus getHttpUserAgent] forHTTPHeaderField:HTTP_HEADER_USER_AGENT];
        
        switch (httpMessage->getContentType()) {
        
            case Message::CNT_XML:
                [urlRequest setValue:HTTP_CONTENT_XML forHTTPHeaderField:HTTP_HEADER_CONTENT_TYPE];
                [urlRequest setValue:HTTP_CONTENT_XML forHTTPHeaderField:HTTP_HEADER_ACCEPT];
                break;
                
            case Message::CNT_JSON:
                [urlRequest setValue:HTTP_CONTENT_JSON forHTTPHeaderField:HTTP_HEADER_CONTENT_TYPE];
                [urlRequest setValue:HTTP_CONTENT_JSON forHTTPHeaderField:HTTP_HEADER_ACCEPT];
                break;
                
            default:
                [urlRequest setValue:HTTP_CONTENT_DEFAULT forHTTPHeaderField:HTTP_HEADER_CONTENT_TYPE];
                [urlRequest setValue:HTTP_CONTENT_DEFAULT_ACCEPT forHTTPHeaderField:HTTP_HEADER_ACCEPT];
                break;
        }
        
        for (std::list<Message::NameValue>::iterator i = m_headers.begin(); i != m_headers.end(); i++) {
            
            NSString* value = [[NSString alloc] initWithUTF8String:i->value.c_str()];
            NSString* header = [[NSString alloc] initWithUTF8String:i->name.c_str()];
            
            [urlRequest setValue:value forHTTPHeaderField:header];
            
            [value release];
            [header release];
        }
        
        std::list<Message::NameValue> headers = httpMessage->getHeaders();
        for (std::list<Message::NameValue>::iterator i = headers.begin(); i != headers.end(); i++) {
            
            NSString* value = [[NSString alloc] initWithUTF8String:i->value.c_str()];
            NSString* header = [[NSString alloc] initWithUTF8String:i->name.c_str()];
            
            [urlRequest setValue:value forHTTPHeaderField:header];
            
            [value release];
            [header release];
        }
        
        NSMutableString* serviceUrl = [[NSMutableString alloc] initWithUTF8String:m_url.c_str()];
        
        switch (httpMessage->getMethod()) {
                
            case http::HttpMessage::POST:  {
                
                NSData* postData = [[NSData alloc] initWithBytes:(void *)request.c_str() length:request.length()];
                [urlRequest setHTTPMethod:HTTP_METHOD_POST];
                [urlRequest setHTTPBody:postData];
                [postData release];
                
                NSString* contentLength = [[NSString alloc] initWithFormat:@"%d", request.length()];
                [urlRequest setValue:contentLength forHTTPHeaderField:@"Content-Length"];
                [contentLength release];
                
                TRACE("  URL..............: %s", [serviceUrl UTF8String]);
                TRACE("  POST BODY........: %s", request.c_str());
                
                break;
            }
            case http::HttpMessage::GET:
            default: {
                
                urlRequest.HTTPMethod = HTTP_METHOD_GET;
                const char* requestString = request.c_str();
                
                if (*requestString == '/')
                    requestString++;
                
                [serviceUrl appendFormat:@"?%@", 
                 [[NSString stringWithUTF8String:requestString] 
                  stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
                
                TRACE("  URL (GET)........: %s", [serviceUrl UTF8String]);
            }
        }
        
        NSURL* url = [NSURL URLWithString:serviceUrl];
        if (!url) {
            
            NSString* errMsg = [[NSString alloc] initWithFormat:@"Invalid URL: %@", serviceUrl];
            response->setError(Message::ERR_CONNECTION_ERROR, 1, [errMsg UTF8String]);
            [errMsg release];
            
            SEND_DATA(response, NULL, 0);
            
            [urlRequest release];
            [serviceUrl release];
            return;
        }
        
        [urlRequest setURL:url];
        [url release];
            
#ifdef HANDLE_COOKIE_HEADER_LIMITATION
        {
            NSAutoreleasePool *mypool = [[NSAutoreleasePool alloc] init];

            NSArray* cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:url];
            if (cookies != nil && [cookies count] > 0) {
                
                TRACE("  COOKIES FOR URL..:");
                
                int size = 0;
                for (NSHTTPCookie* cookie in cookies) {
                    
                    NSString* name = [cookie name];
                    NSString* value = [cookie value];
                    size += [name length] + [value length] + 3;
                    
                    TRACE("    COOKIE.........: '%s' = '%s'", [name UTF8String], [value UTF8String]);
                }
                
                if (size >= MAX_SUPPORTED_COOKIE_HEADER_SIZE) {
                    
                    TRACE("  TOTAL SIZE OF COOKIES %d HAS EXCEEDED THE DEFAULT SIZE IN IOS. CREATING CUSTOM COOKIE HEADER..", size);
                    
                    // Problem is about to take place, generate our own cookie header
                    NSMutableString *cookieHeaderVal = nil; 
                    for (NSHTTPCookie *nextCookie in cookies) {
                        
                        if (!cookieHeaderVal)  {
                            
                            cookieHeaderVal = [[NSMutableString alloc] initWithCapacity:size];
                            [cookieHeaderVal appendFormat:@"%@=%@", [nextCookie name], 
                             [nextCookie value]];
                        } else  {
                            
                            [cookieHeaderVal appendFormat:@"; %@=%@", [nextCookie name], 
                             [nextCookie value]];
                        }
                    }
                    
                    if (cookieHeaderVal)
                    {
                        [urlRequest addValue:cookieHeaderVal forHTTPHeaderField:@"Cookie"];
                        [cookieHeaderVal release];
                    }
                }
            }
            
            [mypool drain];
        }
#else
            
#ifdef LOG_LEVEL_TRACE
        TRACE("  COOKIES FOR URL..:");
        for (NSHTTPCookie* cookie in cookies)
            TRACE("    COOKIE.........: '%s' = '%s'", [[cookie name] UTF8String], [[cookie value] UTF8String]);
#endif
        
#endif
        
        HttpServiceOperation* operation = [ [HttpServiceOperation alloc] 
                                           initWithMessages:message response:response completion:completion timeout:m_timeout ];
        
        NSURLConnection* urlConnection = [ [NSURLConnection alloc]
                                          initWithRequest:urlRequest delegate:operation startImmediately:NO ];
        
        if (!urlConnection) {
            
            NSString* errMsg = [[NSString alloc] initWithFormat:@"Error creating url connection to: %@", serviceUrl];
            response->setError(Message::ERR_CONNECTION_ERROR, 1, [errMsg UTF8String]);
            [errMsg release];
            
            SEND_DATA(response, NULL, 0);
            
            // TODO: Post error message to message bus
            
        } else {
            
            operation.urlConnection = urlConnection;
            [_httpOperationQ addOperation:operation];
            [urlConnection release];
        }
        
        [serviceUrl release];
        [urlRequest release];
        [operation release];
    } 
    else 
    {
        SEND_DATA(response, NULL, 0);
    }
}


#pragma mark -
#pragma mark HttpService implementation

@interface HttpService (Private)

- (id) init;
- (HttpServiceDelegate *) getServiceDelegate;

@end

@implementation HttpService


#pragma mark -
#pragma mark HttpService Initialization

- (id) init {
    
    self = [super init];
    if (!self) return nil;
    
    service = new HttpServiceDelegate();
    
    service->csettingsdelegate = new IOSHttpServiceSettingsHandler();
    service->csettingsdelegate->m_delegate = service;

    service->cdelegate = new IOSHttpService();
    service->cdelegate->m_delegate = service;
    
    service->idelegate = self;
    
    return self;
}

- (void) dealloc {
    
    MessageBusManager* manager = MessageBusManager::instance();
    manager->unregisterService(service->cdelegate);
    
    delete service->csettingsdelegate;
    delete service->cdelegate;
    delete service;
    
    [super dealloc];
}

- (void) setSubject:(NSString *)subject {
    
    service->cdelegate->m_subject = [subject UTF8String];
}

- (void) setUrl:(NSString *)url {
    
    service->cdelegate->m_url = [url UTF8String];
}

- (void) setTimeout:(int)timeout {
    
    service->cdelegate->m_timeout = timeout;
}

- (void) setMethod:(NSString *)method {

    service->cdelegate->m_method = ([method isEqualToString:HTTP_METHOD_GET] ? http::HttpMessage::GET : http::HttpMessage::POST);
}

- (void) setContentType:(NSString *)contentType {

    service->cdelegate->m_contentType = (
        [contentType isEqualToString:HTTP_CONTENT_XML] ? Message::CNT_XML :
        [contentType isEqualToString:HTTP_CONTENT_JSON] ? Message::CNT_JSON : Message::CNT_UNKNOWN );
}

- (void) setRequestTemplate:(NSString *)requestTemplate {
    
    service->cdelegate->m_template = [requestTemplate UTF8String];
}

- (void) addValue:(NSString *)value forHeader:(NSString *)header {
    
    service->cdelegate->m_headers.push_back(Message::NameValue([header UTF8String], [value UTF8String]));
}

- (void) setStreamingSubject:(NSString *)subject {
    
    service->cdelegate->m_streamSubject = [subject UTF8String];
}

- (void) setStreamingKey:(NSString *)key {
    
    service->cdelegate->m_streamKey = [key UTF8String];
}

- (void) setStreamingSnapshot:(NSString *)requestSnapshot {
    
    service->cdelegate->m_streamDoNotSnap = ([requestSnapshot isEqualToString:STR_YES] ? CSTR_FALSE : CSTR_TRUE);
}

- (void) setSubscribeAndSnap:(BOOL)subAndSnap {
    
    service->cdelegate->m_subscribeAndSnap = subAndSnap;
}

- (void) setBindingConfig:(const void *)config {

    if (config)
        service->config = *((boost::shared_ptr<DynaModelBindingConfig> *) config);
}

- (HttpServiceDelegate *) getServiceDelegate {
    
    return service;
}


#pragma -
#pragma Service lifecycle methods

- (void) start {
    
    if (!_trustedDomains) {
        _trustedDomains = [[NSArray alloc] initWithObjects:TRUSTED_DOMAINS, nil];
    }
    
    MessageBusManager* manager = MessageBusManager::instance();
    manager->registerService((Service *) service->cdelegate);
    
    manager->registerListener(CNOTIFICATION_APP_SETTINGS_UPDATE"~smb", service->csettingsdelegate);
}

- (void) stop {
        
    MessageBusManager* manager = MessageBusManager::instance();
    manager->unregisterService((Service *) service->cdelegate);
}

@end
