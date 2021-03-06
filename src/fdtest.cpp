/*
    fdtest.cpp

    FLEXplorer, An explorer for any FLEX file or disk container
    Copyright (C) 1998-2019  W. Schwotzer

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <eh.h>
#include <new.h>
#include <new>
#include <stdexcpt.h>
#include <typeinfo> // needed for typeid

#include "misc1.h"
#include "bdir.h"
#include "bdate.h"
#include "flexerr.h"
#include "fcinfo.h"
#include "fdirent.h"
#include "rfilecnt.h"
#include "dircont.h"

extern void flexContainerTest(const std::string &testPath,
                              FileContainerIf *srcDsk,
                              const std::string &srcFName,
                              FileContainerIf *destDsk,
                              const std::string &destFName,
                              int destType = TYPE_DSK_CONTAINER);

#define TEST_FAILED \
    if (destDsk == nullptr)    \
        fprintf(stderr, "%s test case %d failed\n",  \
                typeid(*srcDsk).name(), testCase);  \
    else   \
        fprintf(stderr, "%s and %s test case %d failed\n",  \
                typeid(*srcDsk).name(), typeid(*destDsk).name(), testCase);

std::string replaceExt(const char *fileName, const char *newExtension)
{
    std::string newName(fileName);
    int         i;

    i = newName.index('.');

    if (i < 0)
    {
        return newName;
    }

    newName.at(0, i, newName);
    return newName + "." + newExtension;
}

int main(int argc, char **argv)
{
    char *testClass;
    int  testCase;

    //BDate::Year2000 = 1;


    /*************************************/
    /* Class std::string test            */
    /*************************************/
    {
        const std::string cs("abc");
        std::string *s, *s1, *s2, s3("aber"), s4;
        const char *p;
        int i;
        FlexDirEntry de;

        s1 = new std::string("cde");
        p = *s1;
        delete s1;
        p = cs;
        p = s3;
        p = s3.c_str();
        p = s4.c_str();
        s = new std::string();
        p = s->c_str();
        delete s;
        s1 = new std::string('a');
        i = s1->length();
        i = s1->capacity();
        i = s1->[0];
        i = s1->[s1->length()-1];
        p = s1->c_str();
        delete s1;
        s2 = new std::string("1234567890abcde");
        i = s2->length();
        i = s2->capacity();
        i = s2->[0];
        i = s2->[s2->length()-1];
        p = s2->c_str();
        p = s2->c_str();
        s2->upcase();
        p = s2->c_str();
        s2->downcase();
        p = s2->c_str();
        delete s2;
        s4 = "xxx";
        p = s4.c_str();
        s3 = s4;
        p = s3.c_str();
        s3 += "abc";
        p = s3.c_str();
        s3 += "cde";
        p = s3.c_str();
        i = s3.capacity();
        p = s3.c_str();
        p = s4.c_str();
        p = s4.c_str();
        p = s4.c_str();
        cat("aber", " nicht doch", s4);
        p = s4.c_str();
        de = FlexDirEntry();
        de.setTotalFileName("file.ext");
        s4 = std::string("abc");
        i = s4.index("b");
        s4 = de.getFileName();
        s4 = de.getTotalFileName();
        s4 = de.getFileExt();
        de.setTotalFileName(".ext");
        s4 = de.getFileName();
        s4 = de.getFileExt();
        de.setTotalFileName("file.");
        s4 = de.getFileName();
        s4 = de.getFileExt();
        de.setTotalFileName("file");
        s4 = de.getFileName();
        s4 = de.getFileExt();
    }


    /*************************************/
    /* Class BDate test               */
    /*************************************/
    {
        BDate date;

        testClass = "BDate";
        testCase = 1;
        date = BDate();
        printf("Date: %s\n", date.getDateString());
        testCase = 1;
        date = BDate::now();
        printf("Date: %s\n", date.getDateString());
    }

    /*************************************/
    /* FlexFileContainer test            */
    /*************************************/

    {
        FileContainerIf *src, *dest;
        std::string testPath, file;
        char        *tempDir;

        tempDir = getenv("TMP");
        testPath = tempDir;
        testPath += PATHSEPARATORSTRING "flextest" PATHSEPARATORSTRING;
        BDirectory::RemoveRecursive(testPath);
        BDirectory::Create(testPath, 755);

        file = testPath + "system.dsk";
        copyFile(".." PATHSEPARATORSTRING "disks" PATHSEPARATORSTRING
                 "system.dsk", file);
        /*
            src  = new FlexFileContainer;
            dest = new FlexFileContainer;
            flexContainerTest(testPath, src, "system.dsk", dest, "test1.dsk",
                              TYPE_DSK_CONTAINER);
            flexContainerTest(testPath, src, "system.dsk", dest, "test1.dsk",
                              TYPE_FLX_CONTAINER);
            delete src;
            delete dest;
            src  = new FlexRamFileContainer;
            dest = new FlexRamFileContainer;
            flexContainerTest(testPath, src, "system.dsk", dest, "test1.dsk",
                              TYPE_DSK_CONTAINER);
            flexContainerTest(testPath, src, "system.dsk", dest, "test1.dsk",
                              TYPE_FLX_CONTAINER);
            delete src;
            delete dest;
        */
        src  = new FlexRamFileContainer;
        dest = new DirectoryContainer;
        flexContainerTest(testPath, src, "system.dsk", dest, "test1.dsk",
                          TYPE_DSK_CONTAINER);
        //flexContainerTest(testPath, src, "system.dsk", dest, "test1.dsk",
        //                  TYPE_DSK_CONTAINER);
        delete src;
        delete dest;

    }
    return 0;
} // end of main



void flexContainerTest(const std::string &testPath,
                       FileContainerIf *srcDsk, const std::string &srcFName,
                       FileContainerIf *destDsk, const std::string &destFName,
                       int destType)

{
    std::string file;
    std::string errMsg;
    char        buffer[1024];
    int         testCase;
    int         i;

    testCase = 1;

    /****************************************************************/
    /* try to open container on a file or directory not available   */
    /****************************************************************/
    try
    {
        file = "." PATHSEPARATORSTRING "_abc_";
        buffer[0] = 0;
        srcDsk->open(file);
        srcDsk->close();
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    errMsg = "Unable to open ";
    errMsg += file;

    if (strlen(buffer) == 0 || strcmp(buffer, errMsg) != 0)
    {
        TEST_FAILED;
    }

    testCase = 2;
    /********************************************************/
    /* try to open file container on a existing directory   */
    /********************************************************/
    {
        char *tempDir;

        tempDir = getenv("TMP");

        try
        {
            buffer[0] = 0;
            srcDsk->open(tempDir);
            srcDsk->close();
        }
        catch (exception &e)
        {
            strcpy(buffer, e.what());
        }

        errMsg = "Unable to open ";
        errMsg += tempDir;

        if (strlen(buffer) == 0 || strcmp(buffer, errMsg) != 0)
        {
            TEST_FAILED;
        }
    }
    testCase = 3;

    /******************************************************************/
    /* try to open a non existing container on the default directory  */
    /******************************************************************/
    try
    {
        file = testPath + "abc.dsk";
        buffer[0] = 0;
        srcDsk->open(file);
        srcDsk->close();
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    errMsg = "Unable to open ";
    errMsg += file;

    if (strlen(buffer) == 0 || strcmp(buffer, errMsg) != 0)
    {
        TEST_FAILED;
    }

    testCase = 4;

    /***************************/
    /* create a new container  */
    /***************************/
    try
    {
        buffer[0] = 0;
        destDsk->create(testPath, destFName, 80, 40, destType);
        destDsk->close();
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    if (strlen(buffer) != 0)
    {
        TEST_FAILED;
    }

    testCase = 6;
    /*****************************************************************/
    /* Write a directory listing of source file to <source file>.TXT */
    /* save all directory entries in an array                        */
    /*****************************************************************/
    {
        FILE        *fp;
        struct stat status;
        std::string file1;
        FlexDirEntry de, *l[1000];
        FlexContainerInfo info;

        try
        {
            fp = nullptr;
            buffer[0] = 0;
            file1 = testPath + replaceExt(srcFName, "TXT");
            fp = fopen(file1, "w");
            file = testPath + srcFName;
            srcDsk->open(file);
            srcDsk->openDirectory("*.*");
            int i = 0;

            while (srcDsk->nextDirEntry(de))
            {
                l[i++] = new FlexDirEntry(de);

                if (fp != nullptr)
                    fprintf(fp, "%12.12s %6d %.12s %.4s\n",
                            de.getTotalFileName().c_str(), de.getSize(),
                            de.getDate().getDateString(),
                            de.getAttributesString().c_str());

                ;
            }

            if (fp != nullptr)
            {
                fclose(fp);
            }

            l[i] = nullptr;
            srcDsk->closeDirectory();
            srcDsk->getInfo(info);
        }
        catch (exception &e)
        {
            strcpy(buffer, e.what());
        }

        srcDsk->close();

        if (strlen(buffer) != 0 || stat(file1, &status) != 0 ||
            status.st_size != 2958)
        {
            TEST_FAILED;
        }
    }

    testCase = 7;
    /********************************************************/
    /* Copy some files from source file to destination file */
    /********************************************************/
    {
        std::string file1;
        FlexDirEntry de, *l[1000];

        try
        {
            buffer[0] = 0;
            file = testPath + srcFName;
            // get all directory entries from srdDsk
            srcDsk->Open(file);
            srcDsk->OpenDirectory("*.*");
            i = 0;

            while (srcDsk->NextDirEntry(de))
            {
                l[i++] = new FlexDirEntry(de);
            }

            l[i] = nullptr;
            srcDsk->CloseDirectory();
            destDsk->Create(testPath, destFName, 80, 40, destType);
            i = 0;

            while (l[i] != nullptr)
            {
                // fprintf(stderr, "%s\n", l[i]->getTotalFileName().c_str());
                srcDsk->FileCopy(l[i]->getTotalFileName(),
                                 l[i]->getTotalFileName(), *destDsk);
                delete l[i];
                i++;
            }

            srcDsk->Close();
            destDsk->Close();
        }
        catch (exception &e)
        {
            strcpy(buffer, e.what());
            srcDsk->close();
            destDsk->close();
        }

        if (strlen(buffer) != 0)
        {
            TEST_FAILED;
        }
    }

    testCase = 8;

    /************************************************/
    /* try to find a file in destination file       */
    /************************************************/
    try
    {
        FlexDirEntry de;
        const char *p;

        buffer[0] = 0;
        file = testPath + destFName;
        destDsk->open(file);
        i = destDsk->findFile("ASN.CMD", de);
        p = de.getDate().getDateString();
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    destDsk->close();

    if (strlen(buffer) != 0 || i == 0)
    {
        TEST_FAILED;
    }

    testCase = 9;

    /************************************************/
    /* try to delete a file in destination file     */
    /************************************************/
    try
    {
        FlexDirEntry de;

        buffer[0] = 0;
        file = testPath + destFName;
        destDsk->open(file);
        destDsk->deleteFile("COPY.CMD");
        i = destDsk->findFile("COPY.CMD", de);
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    destDsk->close();

    if (strlen(buffer) != 0 || i != 0)
    {
        TEST_FAILED;
    }

    testCase = 10;

    /************************************************/
    /* try to rename a file in destination file     */
    /************************************************/
    try
    {
        FlexDirEntry de;

        buffer[0] = 0;
        file = testPath + destFName;
        destDsk->open(file);

        try
        {
            destDsk->deleteFile("CCC.CMD");
        }
        catch (exception &) {};  // ignore errors

        destDsk->renameFile("CAT.CMD", "CCC.CMD");

        i = destDsk->findFile("CCC.CMD", de);
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    destDsk->close();

    if (strlen(buffer) != 0 || i == 0)
    {
        TEST_FAILED;
    }

    try
    {
        FlexDirEntry de;

        buffer[0] = 0;
        i = 0;
        file = testPath + destFName;
        destDsk->open(file);
        destDsk->renameFile("BUILD.CMD", "CLS.CMD");
        i = destDsk->findFile("CLS.CMD", de);
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    destDsk->close();

    if (strlen(buffer) == 0 || i != 0)
    {
        TEST_FAILED;
    }

    testCase = 11;

    /************************************************/
    /* get container info of destination file       */
    /************************************************/
    try
    {
        FlexContainerInfo info;

        buffer[0] = 0;
        file = testPath + destFName;
        destDsk->open(file);
        i = destDsk->getInfo(info);
        destDsk->close();
    }
    catch (exception &e)
    {
        strcpy(buffer, e.what());
    }

    destDsk->close();

    if (strlen(buffer) != 0)
    {
        TEST_FAILED;
    }

}
/*************/
/* end test  */
/*************/
