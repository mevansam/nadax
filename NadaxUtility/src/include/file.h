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
//
// uuid.h : Defines class for generating uuids
//

#ifndef FILE_H_
#define FILE_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>


class File
{
public:
	File();
	File(const char* path);
	virtual ~File();

	const char* getPath();
	void setPath(const char* path);

	bool exists();

	time_t lastAccessed();
	time_t lastModified();

	bool rm();

private:

	char* m_filePath;
};


// **** Implementation ***

inline File::File()
{
	m_filePath = NULL;
}

inline File::File(const char* path)
{
	m_filePath = strdup(path);
}

inline File::~File()
{
	if (m_filePath)
		free(m_filePath);
}

inline const char* File::getPath()
{
	return m_filePath;
}

inline void File::setPath(const char* path)
{
	m_filePath = strdup(path);
}

inline bool File::exists()
{
	bool fileExists = false;
	struct stat fileStat;

	int fd = open(m_filePath, O_RDONLY);
	if (fd != -1)
	{
		fileExists = (fstat(fd, &fileStat) == 0);
	}
	close(fd);

	return fileExists;
}

inline time_t File::lastAccessed()
{
	time_t lastAccessed = 0;

	int fd = open(m_filePath, O_RDONLY);
	if (fd != -1)
	{
		struct stat fileStat;
		if (fstat(fd, &fileStat) == 0)
		{
			lastAccessed = fileStat.st_atimespec.tv_sec;
		}
	}
	close(fd);

	return lastAccessed;
}

inline time_t File::lastModified()
{
	time_t lastModified = 0;

	int fd = open(m_filePath, O_RDONLY);
	if (fd != -1)
	{
		struct stat fileStat;
		if (fstat(fd, &fileStat) == 0)
		{
			lastModified = fileStat.st_mtimespec.tv_sec;
		}
	}
	close(fd);

	return lastModified;
}

inline bool File::rm()
{
	return (remove(m_filePath) != -1);
}

#endif /* FILE_H_ */
