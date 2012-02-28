// The MIT License
//
// Copyright (c) 2011 Mevan Samaratunga
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef PATH_H_
#define PATH_H_

#include <iostream>


namespace binding {


/**
 * Represents a simple path "like" string with
 * ability to do simple wildcard matches.
 */
class Path {

public:

	Path();
	Path(const Path& path);
	Path(const char* path);
	virtual ~Path();

	void reset();

	const char* push(const char* element);
	const char* pop();

	Path& append(Path& path);

	const char* getPathElement(short i);

	const char* str();
	const char* leaf();

	void tag(short n = 1);

	bool isRooted();
	bool isValid();
	bool isEmpty();
	bool isTagged();

	int length();

	bool operator== (const Path& path) { return equals(path); }
	bool operator!= (const Path& path) { return !equals(path); }

	bool operator== (const char* pathStr) { Path path(pathStr); return equals(path); }
	bool operator!= (const char* pathStr) { Path path(pathStr); return !equals(path); }

	bool operator< (const Path& path) const { return strcmp(m_buffer, path.m_buffer) < 0; }
	bool operator> (const Path& path) const { return strcmp(m_buffer, path.m_buffer) > 0; }

	operator const char*() { return m_buffer; }

	friend std::ostream &operator<< (std::ostream& cout, const Path& path);

	void debug(const char* msg);

private:

	void init();
	void addElements(const char* pattern);

	bool equals(const Path& path);
	static bool equals(const Path& a, const Path& b);

	void write(const char* source, int start, int len);
	void reserve(int len);

	char* m_buffer;
	int m_count;
	int m_size;

	int* m_elements;
	int* m_elemLens;
	short* m_flags;

	short m_topIndex;
	short m_tagValue;

	char* m_pathElement;
};

}  // namespace : binding

#endif /* PATH_H_ */
