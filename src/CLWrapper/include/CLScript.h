/**
 * @Author: [lq8@meitu.com] <lynch>
 * @Date:   2019-02-12T19:54:38+08:00
 * @Email:  lq8@meitu.com
 * @Filename: CLScript.h
 * @Last modified by:   lynch
 * @Last modified time: 2019-02-12T19:55:11+08:00
 */



#pragma once

#include <string>
#include <memory>
#include <vector>
#include "CLLog.h"


namespace mtcl
{

class Script {
public:
    Script(){};
    virtual ~Script(){};

public:
    char* load(const char *filePath, size_t &fileSz, bool binary = false)
    {
        if(filePath == NULL)
        {
            LOGE("[%s][%d] filePath is NULL\n", __FILE__, __LINE__);
            return NULL;
        }

        FILE* fd = NULL;
        if (binary)
        {
            if((fd = fopen(filePath, "rb")) == NULL)
            {
                LOGE("[%s][%d] fopen is error.", __FILE__, __LINE__);
                return NULL;
            }
        }
        else
        {
            if((fd = fopen(filePath, "r")) == NULL)
            {
                LOGE("[%s][%d] fopen is error.", __FILE__, __LINE__);
                return NULL;
            }
        }

        if(0 != fseek(fd, 0, SEEK_END))
        {
            fclose(fd);
            LOGE("fseek error: %s,%d", __FILE__, __LINE__);
            return NULL;
        }
        fileSz = ftell(fd);

        if(fileSz <= 0)
        {
            fclose(fd);
            LOGE("ftell error: %s,%d",__FILE__,__LINE__);
            return NULL;
        }

        if(0 != fseek(fd, 0, SEEK_SET))
        {
            fclose(fd);
            LOGE("fseek error: %s,%d",__FILE__,__LINE__);
            return NULL;
        }

        // std::shared_ptr<char> shared_data(new char(fileSz + 1));
        // char* cSourceString = shared_data.get();
        char* cSourceString = (char*)malloc(sizeof(char)*(fileSz + 1));
        // char* cSourceString = new char[fileSz + 1];

        if(NULL == cSourceString)
        {
            fclose(fd);
            LOGE("malloc error: %s,%d",__FILE__,__LINE__);
            return NULL;
        }

        if (fread(cSourceString, sizeof(char), fileSz, fd) != fileSz)
        {
            fclose(fd);
            free(cSourceString);
            LOGE("fread error: %s,%d",__FILE__,__LINE__);
            return NULL;
        }
        cSourceString[fileSz] = '\0';
        fclose(fd);

        return cSourceString;
    }

    //加解密用同一个函数密用
    bool enCrypt(char* pData, int nSize)
    {
        //////////////////////////////////////////////////////////////////////////
        //【校验参数】
        if (0==pData)				//数据指针不能为NULL
        {
            return false;
        }

        if (nSize < 1)				//数据个数不能小于1
        {
            return false;
        }

        //得到数据流总共有多少个整型大小，以及不足整型大小的字节个数
        int nCountInt = nSize / 4;
        int nCountYu = nSize % 4;

        /////////////////////////////////////
        //对整型大小的数据进行加密
        char* pDataTemp = pData;				//临时数据流指针
        while(nCountInt)
        {
            *(int*)pDataTemp ^= 0x3AB9347C;	//异或加密运算
            pDataTemp = pDataTemp + 4;		//指向下一个整型值
            nCountInt--;						//整型个数计数器减1
        }

        //对不足整型大小的数据进行解密
        if (nCountYu>0)						   //判断是否有不足4字节的数据
        {
            if (1==nCountYu)					//如果有剩余1个字节
            {
                *pDataTemp ^= 0x3A;
            }
            else if(2==nCountYu)				//如果有剩余2个字节
            {
                *(short*)pDataTemp ^= 0x3AB9;
            }
            else if(3==nCountYu)				//如果有剩余3个字节
            {
                *(short*)pDataTemp ^= 0x3AB9;
                *(pDataTemp+2) ^= 0x34;
            }
        }

        return true;
    }

    bool codeScript(const char *srcPath, const char *dstPath)
    {
        bool ret = true;
        FILE *stream = NULL;
        FILE *dstStream = NULL;

        stream = fopen(srcPath, "r" );
        dstStream = fopen(dstPath, "wb" );
        if (stream == NULL)
        {
            LOGE("stream open fail! %s,%d",__FILE__, __LINE__);
            ret = false;
        }
        else
        {
            fseek(stream, 0, SEEK_END);
            const int size = ftell(stream);
            fseek(stream, 0, SEEK_SET);

            if (dstStream == NULL)
            {
                fclose(stream);
                ret = false;
                LOGE("dstStream open fail! %s,%d",__FILE__, __LINE__);
            }
            else
            {
                char * data = new char[size];
                if (data)
                {
                    // 从文件中读数据
                    fread(data, 1, size, stream);
                    // 加密代码
                    enCrypt(data, size);
                    // 将加密过的数据写入文件中
                    fwrite(data, 1, size, dstStream);
                    delete[] data;
                }
                else
                {
                    ret = false;
                }

                fclose(stream);
                fclose(dstStream);
            }
        }

        return ret;
    }

    bool codeScriptWithString(char *data, const int size, const char *dstPath)
    {
        bool ret = true;
        FILE *dstStream = NULL;

        dstStream = fopen(dstPath, "wb" );
        if (dstStream == NULL)
        {
            ret = false;
            LOGE("dstStream open fail! %s,%d",__FILE__, __LINE__);
        }
        else
        {
            if (data != NULL)
            {
                // 加密代码
                enCrypt(data, size);
                // 将加密过的数据写入文件中
                fwrite(data, 1, size, dstStream);
            }
            else
            {
                ret = false;
                LOGE("script string is null! %s,%d",__FILE__, __LINE__);
            }
            fclose(dstStream);
        }

        return ret;
    }

};

} // end namespace mtcl
