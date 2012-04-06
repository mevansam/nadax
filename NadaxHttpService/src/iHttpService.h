//
//  HttpService.h
//  Framework
//
//  Created by Mevan Samaratunga on 5/27/11.
//  Copyright 2011 Fidelity Investments. All rights reserved.
//

#import "ServiceLifecycle.h"

typedef struct HttpServiceDelegate HttpServiceDelegate;


#pragma mark -
#pragma mark Abstraction for all HttpService instances

@interface HttpService : NSObject<ServiceLifecycle> {
    
@private

    HttpServiceDelegate* service;
}


# pragma -
# pragma HttpService configuration properties

- (void) setSubject:(NSString *)subject;

- (void) setUrl:(NSString *)url;
- (void) setTimeout:(int)timeout;

- (void) setMethod:(NSString *)method;
- (void) setContentType:(NSString *)contentType;
- (void) setRequestTemplate:(NSString *)requestTemplate;

- (void) addValue:(NSString *)value forHeader:(NSString *)header;

- (void) setStreamingSubject:(NSString *)subject;
- (void) setStreamingKey:(NSString *)key;
- (void) setStreamingSnapshot:(NSString *)requestSnapshot;
- (void) setSubscribeAndSnap:(BOOL)subAndSnap;

- (void) setBindingConfig:(const void *)config;

@end
