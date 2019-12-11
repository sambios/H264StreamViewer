
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <memory.h>
#include <QTreeWidget>
#include "nal_parse.h"

CNalParser::CNalParser()
{
    m_nType = FILE_H264; // default
    m_filename = NULL;
    m_hH264 = NULL;
    m_hH265 = NULL;
    m_naluData = NULL;
    dlg = NULL;
    m_pTree = NULL;
    m_pFile = NULL;

    memset(m_tmpStore, '\0', 1024);
    memset(m_outputInfo, '\0', OUTPUT_SIZE);
}

CNalParser::~CNalParser()
{
    release();
}

int CNalParser::init(const char* filename, QTreeWidget* tree)
{
    m_filename = filename;

    // judge file 
    m_nType = judeVideoFile(m_filename);

    if (m_nType == FILE_UNK)
    {
        return -1;
    }
    // init handle
    else if (m_nType == FILE_H265)
    {
        if (m_hH265 != NULL)
        {
            h265_free(m_hH265);
        }
        m_hH265 = h265_new();
    }
    else
    {
        if (m_hH264 != NULL)
        {
            h264_free(m_hH264);
        }
        m_hH264 = h264_new();
    }
    
    if (tree != NULL)
    {
        m_pTree = tree;
        //m_pTree->DeleteAllItems();
    }

    if (m_pFile != NULL)
    {
        fclose(m_pFile);
        m_pFile = NULL;
    }

    m_pFile = fopen(m_filename, "r+b");

    return 0;
}

int CNalParser::release(void)
{
    if (m_hH264 != NULL)
    {
        h264_free(m_hH264);
        m_hH264 = NULL;
    }
    if (m_hH265 != NULL)
    {
        h265_free(m_hH265);
        m_hH265 = NULL;
    }
    if (m_pFile != NULL)
    {
        fclose(m_pFile);
        m_pFile = NULL;
    }

    return 0;
}

int CNalParser::probeNALU(vector<NALU_t>& vNal, int num)
{
    NALU_t n;
    int nal_num=0;
    int offset=0;
    int nalLen = 0;

    memset(&n, '\0', sizeof(NALU_t));

    n.type = m_nType;

    offset = findFirstNALU(m_pFile, &(n.startcodeLen));

    if (offset < 0)
    {
        return -1;
    }
    fseek(m_pFile, offset, SEEK_SET);
    while (!feof(m_pFile))
    {
        if (num > 0 && nal_num == num)
        {
            break;
        }
        nalLen = getAnnexbNALU(m_pFile, &n);//ÿִ��һ�Σ��ļ���ָ��ָ�򱾴��ҵ���NALU��ĩβ����һ��λ�ü�Ϊ�¸�NALU����ʼ��0x000001
        n.offset = offset;
        n.num = nal_num;
        offset = offset + nalLen;

        vNal.push_back(n);

        nal_num++;
    }
    return 0;
}

int CNalParser::parseNALU(NALU_t& vNal, char** naluData, char** naluInfo)
{
    if (m_naluData == NULL)
    {
        free(m_naluData);
        m_naluData = NULL;
    }
    m_naluData = (uint8_t *)malloc(vNal.len);

    fseek(m_pFile, vNal.offset, SEEK_SET);
    fread(m_naluData, vNal.len, 1, m_pFile);

    // ����Ҫ�ٴβ�ѯnal
    //find_nal_unit(m_naluData, vNal.len, &nal_start, &nal_end);
    memset(m_outputInfo, '\0', OUTPUT_SIZE); // ������������
    if (m_nType == 1)
    {
        // �˺�������ʱ���ǲ�����startcode�ģ�����Ҫ��ȥstartcodeLen
        h265_read_nal_unit(m_hH265, &m_naluData[vNal.startcodeLen], vNal.len - vNal.startcodeLen);
        h265_debug_nal_t(m_hH265,m_hH265->nal);    // ��ӡ��m_outputInfo��
    }
    else
    {
        read_nal_unit(m_hH264, &m_naluData[vNal.startcodeLen], vNal.len - vNal.startcodeLen);
        h264_debug_nal_t(m_hH264, m_hH264->nal);  // ��ӡ��m_outputInfo��
    }

    *naluData = (char*)m_naluData;
    *naluInfo = m_outputInfo;

    return 0;
}

static int ue(char *buff, int len, int &start_bit)
{
    int zero_num = 0;
    int ret = 0;

    while (start_bit < len * 8)
    {
        if (buff[start_bit / 8] & (0x80 >> (start_bit % 8)))
        {
            break;
        }
        zero_num++;
        start_bit++;
    }
    start_bit++;

    for (int i=0; i<zero_num; i++)
    {
        ret <<= 1;
        if (buff[start_bit / 8] & (0x80 >> (start_bit % 8)))
        {
            ret += 1;
        }
        start_bit++;
    }
    return (1 << zero_num) - 1 + ret;
}

/**
����NAL������������ʼ�ַ�֮�������ֽ�����������startcode��NALU�ĳ���

note��һ����Ƶ�ļ��в�ͬ��NAL��startcode���ܲ�һ��������SPSΪ4�ֽڣ���SEI����Ϊ3�ֽ�
todo:ÿ�ζ�һ���ֽڣ����������޺õķ�����
*/

int CNalParser::getAnnexbNALU(FILE* fp, NALU_t* nalu)
{
    int pos = 0;
    int found, rewind;
    unsigned char *buffer;
    int info2=0, info3=0;
    int eof = 0;

    if ((buffer = (unsigned char*)calloc (MAX_NAL_SIZE, sizeof(char))) == NULL)
        printf("Could not allocate buffer memory\n");

    if (3 != fread (buffer, 1, 3, fp))//�������ж�3���ֽ�
    {
        free(buffer);
        return 0;
    }
    info2 = findStartcode3(buffer);//�ж��Ƿ�Ϊ0x000001
    if(info2 != 1)
    {
        //������ǣ��ٶ�һ���ֽ�
        if(1 != fread(buffer+3, 1, 1, fp))//��һ���ֽ�
        {
            free(buffer);
            return 0;
        }
        info3 = findStartcode4(buffer);//�ж��Ƿ�Ϊ0x00000001
        if (info3 != 1)//������ǣ�����-1
        {
            free(buffer);
            return -1;
        }
        else
        {
            //�����0x00000001,�õ���ʼǰ׺Ϊ4���ֽ�
            nalu->startcodeLen = 4;
        }
    }
    else
    {
        //�����0x000001,�õ���ʼǰ׺Ϊ3���ֽ�
        nalu->startcodeLen = 3;
    }

    pos = nalu->startcodeLen;
    //������һ����ʼ�ַ��ı�־λ
    found = 0;
    info2 = 0;
    info3 = 0;

    while (!found)
    {
        if (feof(fp))//�ж��Ƿ����ļ�β
        {
            eof = 1;
            goto got_nal;
        }
        buffer[pos++] = fgetc(fp);//��һ���ֽڵ�BUF��

        info3 = findStartcode4(&buffer[pos-4]);//�ж��Ƿ�Ϊ0x00000001
        if(info3 != 1)
            info2 = findStartcode3(&buffer[pos-3]);//�ж��Ƿ�Ϊ0x000001

        found = (info2 == 1 || info3 == 1);
    }

    // startcode����Ϊ3��Ҳ����Ϊ4����Ҫ����ж�
    rewind = (info3 == 1)? -4 : -3;

    if (0 != fseek (fp, rewind, SEEK_CUR))//���ļ�ָ��ָ��ǰһ��NALU��ĩβ
    {
        free(buffer);
        printf("Cannot fseek in the bit stream file");
    }

got_nal:
    // ���ﵽ�ļ�ĩβʱ������1��λ��
    if (eof)
    {
        rewind = -1;
    }

    // ������ʼ�����ڵ�5���ֽ�
    if (nalu->startcodeLen == 3)
        sprintf(nalu->startcodeBuffer, "%02x%02x%02x%02x", buffer[0], buffer[1], buffer[2], buffer[3]);
    else
        sprintf(nalu->startcodeBuffer, "%02x%02x%02x%02x%02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4]);
    nalu->len = pos+rewind;

    uint8_t nal_header = 0;
    if (nalu->type)
    {
        m_hH265->sh->read_slice_type = 1;
        h265_read_nal_unit(m_hH265, &buffer[nalu->startcodeLen], nalu->len - nalu->startcodeLen);
        nalu->nalType = m_hH265->nal->nal_unit_type;
        nalu->sliceType = m_hH265->sh->slice_type;
        m_hH265->sh->read_slice_type = 0;
    }
    else
    {
    // simple version
#if 0
        nal_header = buffer[nalu->startcodeLen];
        nalu->nalType = nal_header & 0x1f;// 5 bit

        // ��ȡslice���ͣ�I֡��P֡��B֡
        // ע����nal����Ϊ1~5ʱ��ȡ
        if (nalu->nalType <= 5 && nalu->nalType >= 1)
        {
            int start_bit = 0;
            int first_mb_in_slice = ue((char*)buffer+nalu->startcodeLen+1, 8, start_bit);
            nalu->sliceType = ue((char*)buffer+nalu->startcodeLen+1, 8, start_bit);
        }
        if (nalu->nalType == 7 || nalu->nalType == 8) // sps pps
        {
            read_nal_unit(m_hH264, &buffer[nalu->startcodeLen], nalu->len - nalu->startcodeLen);
        }
#else
        m_hH264->sh->read_slice_type = 1;
        read_nal_unit(m_hH264, &buffer[nalu->startcodeLen], nalu->len - nalu->startcodeLen);
        nalu->nalType = m_hH264->nal->nal_unit_type;
        nalu->sliceType = m_hH264->sh->slice_type;
        m_hH264->sh->read_slice_type = 0;
#endif
    }

    free(buffer);

    return (pos+rewind);//����������ʼ�ַ�֮�������ֽ�������������ǰ׺��NALU�ĳ���
}

int CNalParser::findFirstNALU(FILE* fp, int* startcodeLenght)
{
    int found = 0;
    int info2 = 0;
    int info3 = 0;
    int eof = 0;
    int pos = 0;
    int startcode_len = 0;
    unsigned char *buffer = NULL;

    if ((buffer = (unsigned char*)calloc(MAX_NAL_SIZE, sizeof(char))) == NULL)
        printf ("Could not allocate buffer memory\n");

    while (!found && !feof(fp))
    {
        buffer[pos++] = fgetc(fp);//��һ���ֽڵ�BUF��

        info3 = findStartcode4(&buffer[pos-4]);//�ж��Ƿ�Ϊ0x00000001
        if(info3 != 1)
        {
            info2 = findStartcode3(&buffer[pos-3]);//�ж��Ƿ�Ϊ0x000001
            if (info2)
            {
                startcode_len = 3;
            }
        }
        else
        {
            startcode_len = 4;
        }

        found = (info2 == 1 || info3 == 1);
        if (pos >= MAX_NAL_SIZE)
        {
            free(buffer);
            return -1;
        }
    }

    // �ļ�ָ��Ҫ�ָ�
    fseek(fp, -startcode_len, SEEK_CUR);

    free(buffer);
    if (startcodeLenght != NULL)
        *startcodeLenght = startcode_len;

    return pos - startcode_len;
}

FileType CNalParser::judeVideoFile(const char* filename)
{
    char szExt[16] = {0};
    FileType type = FILE_H264; // default

    //_splitpath(filename, NULL, NULL, NULL, szExt);
    if (!strcmp(&szExt[1], "h265") || !strcmp(&szExt[1], "265") ||
        !strcmp(&szExt[1], "hevc"))
    {
        type = FILE_H265;
    }
    else if (!strcmp(&szExt[1], "h264") || !strcmp(&szExt[1], "264") ||
        !strcmp(&szExt[1], "avc"))
    {
        type = FILE_H264;
    }
    else
    {
        // read content 
        FILE* fp = NULL;
        int offset = 0;
        int startcode = 0;
        unsigned char nalHader = 0;
        unsigned char nalType = 0;

        fp = fopen(filename, "r+b");
        offset = findFirstNALU(fp, &startcode);
        if (offset < 0)
        {
            return FILE_UNK;
        }
        fseek(fp, offset+startcode, SEEK_SET);
        fread((void*)&nalHader,1,1,fp);
        // check h264 first...
        nalType = nalHader & 0x1f; // 5 bit
        if (nalType > 0 && nalType < 22) // ok
        {
            type = FILE_H264;
        }
        else
        {
            // not h264, then check h265...
            nalType = (nalHader>>1) & 0x3f; // 6 bit
            if (nalType >= 0 && nalType <= 47) // ok
            {
                type = FILE_H265;
            }
        }
        fclose(fp);
    }

    return type;
}
