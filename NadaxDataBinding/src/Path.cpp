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

#include "Path.h"

#include <malloc/malloc.h>
#include <iomanip>

#define BLOCK_SIZE  32

#define WILD_ROOT     0x01
#define WILD_ELEMENT  0x02

#define PATH_SEP  "/",0,1


namespace binding {


Path::Path() {

	init();
}

Path::Path(const Path& path) {

	init();
	write(path.m_buffer, 0, path.m_count);

	int numElements = path.m_topIndex + 1;
	memcpy(m_elements, path.m_elements, numElements * sizeof(int));
	memcpy(m_elemLens, path.m_elemLens, numElements * sizeof(int));
	memcpy(m_flags, path.m_flags, numElements * sizeof(short));

    m_topIndex = path.m_topIndex;
}

Path::Path(const char* path) {

	init();
	addElements(path);
}

Path::~Path() {

	free(m_buffer);
	free(m_elements);
	free(m_elemLens);
	free(m_flags);

	if (m_pathElement)
		free(m_pathElement);
}

void Path::init() {

	m_buffer = (char *) calloc(BLOCK_SIZE, sizeof(char));
	m_elements = (int *) calloc(64, sizeof(int));
	m_elemLens = (int *) calloc(64, sizeof(int));
	m_flags = (short *) calloc(64, sizeof(int));

	m_size = BLOCK_SIZE;
	m_count = 0;

	m_topIndex = 0;
	m_tagValue = 0;

	m_pathElement = NULL;
}

void Path::reset() {

	memset(m_buffer, 0, m_size * sizeof(char));
	memset(m_elements, 0, 64 * sizeof(int));
	memset(m_elemLens, 0, 64 * sizeof(int));
	memset(m_flags, 0, 64 * sizeof(short));

	m_count = 0;

	m_topIndex = 0;
	m_tagValue = 0;

	if (m_pathElement) {

		free(m_pathElement);
		m_pathElement = NULL;
	}
}

const char* Path::push(const char* element) {

    if (m_tagValue > 0)
        ++m_tagValue;

    if (m_count > 1 || (m_count == 1 && *m_buffer != '/'))
        write(PATH_SEP);

    addElements(element);
    return m_buffer;
}

const char* Path::pop() {

    if (m_tagValue > 0)
        --m_tagValue;

    const char* top = leaf();

    if (m_topIndex == 0)
    {
    	*m_buffer = 0;
        m_count = 0;

        m_elements[0] = 0;
        m_elemLens[0] = 0;
        m_flags[0] = 0;
    }
    else
    {
        m_count = m_elements[m_topIndex--] - 1;
        *(m_buffer + m_elements[m_topIndex] + m_elemLens[m_topIndex]) = 0;
    }

    return top;
}

void Path::addElements(const char* pattern) {

	write(pattern, 0, strlen(pattern));
	*(m_buffer + m_count) = 0;

    int i, j, k;
    char ch;

    i = m_topIndex;
    int start = m_elements[i] + m_elemLens[i];

    for (i = start; i <= m_count; i++) {

        if (i == m_count || m_buffer[i] == '/') {

            j = m_topIndex;
            k = m_elements[j];

            if ( (m_elemLens[j] = i - k) == 1 ) {

                ch = m_buffer[k];

                if (k == 0 && ch == '*')
                    m_flags[j] = WILD_ROOT;
                else if (ch == '?')
                    m_flags[j] = WILD_ELEMENT;
            }

            if (i < m_count)
                m_elements[++m_topIndex] = i + 1;
        }
    }
}

Path& Path::append(Path& path) {

    if (path.m_count > 0) {

        int i = (*path.m_buffer == '/' ? 1 : 0);
        int j, k;

        for (; i <= path.m_topIndex; i++) {

            if (i != 0 || m_count != 0) {

                write(PATH_SEP);
                m_elements[++m_topIndex] = m_count;
            }

            j = path.m_elements[i];
            k = (i == path.m_topIndex ? path.m_count : path.m_elements[i + 1] - 1) - j;

            m_flags[m_topIndex] = path.m_flags[i];
            m_elemLens[m_topIndex] = k;

            write(path.m_buffer, j, k);
        }
    }

    return *this;
}

const char* Path::getPathElement(short i) {

	if (m_pathElement)
		free(m_pathElement);

	m_pathElement = (char *) malloc(m_elemLens[i] + 1);
	memcpy(m_pathElement, m_buffer + m_elements[i], m_elemLens[i]);
	*(m_pathElement + m_elemLens[i]) = 0;

	return m_pathElement;
}

const char* Path::str() {

	return m_buffer;
}

const char* Path::leaf() {

	return m_buffer + m_elements[m_topIndex];
}

void Path::tag(short n) {

	m_tagValue = n;
}

bool Path::isRooted() {

	return m_count > 0 && *m_buffer == '/';
}

bool Path::isValid() {

	if (*m_buffer == 0 && m_topIndex == 0)
		return true;

	if (isRooted() && (m_elements[0] != 0 || m_elements[1] != 1))
		return false;

	if (!isRooted() && m_elemLens[0] == 0)
		return false;

	return true;
}

bool Path::isEmpty() {

	return (m_topIndex == 0 && m_count == 0);
}

bool Path::isTagged() {

	return (m_tagValue > 0);
}

int Path::length() {

	if (m_count == 0)
		return 0;
	else if (*m_buffer == '/')
		return *(m_buffer + 1) ? m_topIndex : 0;
	else
		return m_topIndex + 1;
}

bool Path::equals(const Path& path) {

	if (m_count == 0 && path.m_count == 0)
	{
		return true;
	}
	else if (m_count == 0 || path.m_count == 0)
	{
		return false;
	}

	if (path.m_topIndex < m_topIndex)
	{
		return Path::equals(path, *this);
	}
	else
	{
		return Path::equals(*this, path);
	}

	return false;
}

/*
 * Support method for equals(), above.  Assumes that
 * a.length <= b.length.
 */
inline bool Path::equals(const Path& a, const Path& b)
{
    if (a.m_buffer[0] == '/' && a.m_topIndex < b.m_topIndex)
    {
        return false;
    }

    // a is now unrooted or the same length as b.
    // check equality of each element and we're done.

    int aindex, aoffset, aend = a.m_count;
    int bindex, boffset, bend = b.m_count;

    aindex = a.m_topIndex;
    bindex = b.m_topIndex;

    while (aindex >= 0)
    {
        aoffset = a.m_elements[aindex];
        boffset = b.m_elements[bindex];

        aend = (aindex == a.m_topIndex ? a.m_count : a.m_elements[aindex + 1] - 1);
        bend = (bindex == b.m_topIndex ? b.m_count : b.m_elements[bindex + 1] - 1);

        if ( !(a.m_flags[aindex] & 0x03) &&
            !(b.m_flags[bindex] & 0x03) )
        {
            if ( (aend - aoffset) == (bend - boffset) )
            {
                if (strncmp(a.m_buffer + aoffset, b.m_buffer + boffset, aend - aoffset) != 0)
                {
					return false;
                }
            }
            else
            {
                return false;
            }
        }

        aindex--;
        bindex--;
    }

    if (bindex >= 0 && !(a.m_flags[0] & 0x01))
    {
        // Handle special wild card case: i.e. b/c == */b/c
        return (bindex == 0 && (b.m_flags[bindex] & 0x01));
    }

    return true;
}

void Path::debug(const char* msg) {

	std::cout << std::endl;
	std::cout << "Debug output for Path instance '" << msg;
	std::cout << "' - size=" << m_size;
	std::cout << ", count=" << m_count;
	std::cout << ", topIndex=" << m_topIndex;
	std::cout << ", tagValue=" << m_tagValue;
	std::cout << ", length=" << length() << std::endl;

	int i, j, k;

	std::cout << "    buffer : " << m_buffer << std::endl;
	std::cout << "  elements : ";

	k = 0;
	for (i = 0; i <= m_topIndex; i++) {

		for (j = k; j < m_elements[i]; j++)
			std::cout << " ";

		std::cout << "|";
		k = m_elements[i] + 1;

		if (m_flags[i] & 0x03) {

			std::cout << "W";
			++k;
		}
	}

	std::cout << std::endl;
	std::cout << "  elemLens : ";
	std::cout << std::setfill(' ') << std::left;

	for (i = 0; i <= m_topIndex; i++) {

		if (i == 0 && m_elements[0] > 0)
			std::cout << std::setw(m_elements[0]) << "";

		std::cout << std::setw(m_elemLens[i] + 1) << m_elemLens[i];
	}

	std::cout << std::endl;
	std::cout.flush();
}

void Path::write(const char* source, int start, int len) {

	reserve(m_count + len);
	memcpy(m_buffer + m_count, source + start, len);
	m_count += len;
    *(m_buffer + m_count) = 0;
}

void Path::reserve(int len) {

	if (++len > m_size) {

		int numBlocks = ((m_count + len) / BLOCK_SIZE + 1);
		char* newBuffer = (char *) calloc(numBlocks * BLOCK_SIZE, sizeof(char));

		if (m_count)
			memcpy(newBuffer, m_buffer, m_count);

		free(m_buffer);
		m_buffer = newBuffer;

		m_size = numBlocks * BLOCK_SIZE;
	}
}

std::ostream& operator<< (std::ostream& cout, const Path& path) {

	cout << path.m_buffer;
	return cout;
}


}  // namespace : binding
