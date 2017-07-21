// VideoOpenCV.cpp : 定义控制台应用程序的入口点。
//
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fstream>
#include "VideoCV.h"

using namespace ThreadSpace;

cv::Mat org,dst,img,tmp,tips;
int gPos = 0;

void on_track(int pos,void *ustc)
{
    gPos = pos;
}

void on_mouse(int event,int x,int y,int flags,void *ustc)//event鼠标事件代号，x,y鼠标坐标，flags拖拽和键盘操作的代号  
{  
    static Point pre_pt = (-1,-1);//初始坐标  
    static Point cur_pt = (-1,-1);//实时坐标  
    static Point rect_pt = (-1,-1); //矩形区域坐标
    static Point tips_pt = (-1,-1); //提示语坐标
    char   temp[256] = {0}; 
    int    value = 0;
    STRING_T  strTips = STRING_T();

    UserData_S *psData = (UserData_S*)ustc;
    STRING_T     strComplete = psData->strName;
    INT32_T      i32NameLen = 0;
    STRING_T     strTmpFileName = STRING_T();
    STRING_T     strFileSuffix = STRING_T();


    if (event == CV_EVENT_LBUTTONDOWN)//左键按下，读取初始坐标，并在图像上该点处划圆  
    {  
        org.copyTo(img);              //将原始图片复制到img中  
        sprintf(temp,"(%d,%d)",x,y);  
        pre_pt = Point(x,y);  
        putText(img,temp,pre_pt,FONT_HERSHEY_SIMPLEX,0.5,Scalar(0,0,0,255),1,8);//在窗口上显示坐标  
        circle(img,pre_pt,2,Scalar(255,0,0,0),CV_FILLED,CV_AA,0);               //划圆  
        imshow("img",img);  
    }  
    else if (event == CV_EVENT_MOUSEMOVE && !(flags & CV_EVENT_FLAG_LBUTTON))//左键没有按下的情况下鼠标移动的处理函数  
    {  
        img.copyTo(tmp);                                                    //将img复制到临时图像tmp上，用于显示实时坐标  
        sprintf(temp,"(%d,%d)",x,y);  
        cur_pt = Point(x,y); 
        rect_pt = Point(x+120,y+120); 
        putText(tmp,temp,cur_pt,FONT_HERSHEY_SIMPLEX,0.5,Scalar(0,0,0,255));//只是实时显示鼠标移动的坐标  
        rectangle(tmp,cur_pt,rect_pt,Scalar(0,255,0,0),1,8,0); //实时显示一个矩形区域
        imshow("img",tmp);  
    }  
    else if (event == CV_EVENT_LBUTTONUP)//左键松开，将在图像上划矩形  
    {  
        org.copyTo(img);  
        circle(img,pre_pt,2,Scalar(255,0,0,0),CV_FILLED,CV_AA,0);  
        rectangle(img,pre_pt,rect_pt,Scalar(0,255,0,0),1,8,0);//根据初始点和结束点，将矩形画到img上    
        imshow("img",img);  
        img.copyTo(tmp);

        //截取矩形包围的图像，并保存到dst中  
        int width = abs(pre_pt.x - rect_pt.x);  
        int height = abs(pre_pt.y - rect_pt.y);  
        if (width == 0 || height == 0)  
        {  
            printf("width == 0 || height == 0 \n");  
            return;  
        }

        if (rect_pt.x > img.cols || rect_pt.y > img.rows)
        {
            printf("over zone!!! \n");
            return;
        }

        dst = org(Rect(min(rect_pt.x,pre_pt.x),min(rect_pt.y,pre_pt.y),120,120)); //width,height
        //namedWindow("dst");
        //imshow("dst",dst); 

        //namedWindow("bar");
        //createTrackbar("bar","bar",&value,1000,on_track);
        //tips = imread(psData->strTipsName);
        //tips_pt = Point(10,50);
        //strTips = "if bar == 0:normal driver;  ";
        //strTips += "if bar in [1,300):smoking; ";
        //strTips += "if bar in [300,600):calling;";
        //strTips += "if bar in > 600: other";

        //putText(tips,strTips,tips_pt,FONT_HERSHEY_SIMPLEX,0.5,Scalar(0,0,0,255));//显示一行提示语 
        //imshow("bar",tips);
        //waitKey();

        if (gPos == 100)
        {
            strTmpFileName = "result\\all\\";
            strFileSuffix = "a.png";
        } 
        else if(gPos < 300)
        {
            strTmpFileName = "result\\smoking\\";
            strFileSuffix = "s.png";
        }
        else if (gPos < 600)
        {
            strTmpFileName = "result\\calling\\";
            strFileSuffix = "c.png";
        }
        else
        {
            strTmpFileName = "result\\other\\";
            strFileSuffix = "o.png";
        }
        gPos = 0;

        i32NameLen = strComplete.length();
        strComplete.replace(i32NameLen-4,4,strFileSuffix);
        strComplete.replace(14,4,strTmpFileName);
        imwrite(strComplete,dst); 
    }  
}

/*根据文件路径获取文件名*/
static void getJustCurrentFile(STRING_T path, vector<STRING_T>& files)  
{  
    INT32_T   hFile   =   0;
    struct    _finddata_t fileinfo;  
    STRING_T  strTmp;  
    if((hFile = _findfirst(strTmp.assign(path).append("\\*").c_str(),&fileinfo)) !=  -1)  
    {  
        do  
        {   
            if(!(fileinfo.attrib &  _A_SUBDIR))   
            {  
                files.push_back(fileinfo.name);
            }   
        }while(_findnext(hFile, &fileinfo)  == 0);  
        _findclose(hFile); 
    } 
}

//图像像素压缩为指定规格（用于获取样本数据）
static INT32_T ZipPngNormal(STRING_T strPngPath,StringList_V &vPngFile,INT32_T i32cols,INT32_T i32rows)
{
    STRING_T     strFileName = STRING_T();
    STRING_T     strTmp = STRING_T();
    INT32_T      i32FileNum = 0;
    Mat          mSrc,mDest;

    i32FileNum = vPngFile.size();
    if ( 0 == i32FileNum)
    {
        return EXIT_ERR;
    }

    for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum;i32Index++)
    {
        strFileName = strPngPath + vPngFile[i32Index];
        mSrc = imread(strFileName,IMREAD_GRAYSCALE);
        resize(mSrc,mDest,Size(i32cols,i32rows),0,0,INTER_LINEAR);
        ::DeleteFile(strFileName.c_str());

        strTmp = strPngPath + "gray\\" + vPngFile[i32Index];
        imwrite(strTmp,mDest);
    }
    return EXIT_OK;
}

//图像像素压缩为原来的一半
static INT32_T ZipPng(STRING_T strPngPath,StringList_V &vPngFile,INT32_T i32cols,INT32_T i32rows)
{
    STRING_T     strFileName = STRING_T();
    INT32_T      i32FileNum = 0;
    Mat          mSrc,mDest;

    i32FileNum = vPngFile.size();
    if ( 0 == i32FileNum)
    {
        return EXIT_ERR;
    }

    for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum;i32Index++)
    {
        strFileName = strPngPath + vPngFile[i32Index];
#if 1
        mSrc = imread(strFileName,IMREAD_GRAYSCALE);
#else
        mSrc = imread(strFileName,IMREAD_COLOR);
#endif
        resize(mSrc,mDest,Size(i32cols,i32rows),0,0,INTER_LINEAR);
        ::DeleteFile(strFileName.c_str());
        imwrite(strFileName,mDest);
    }
    return EXIT_OK;
}

//从视频文件中获取jpg图片
static INT32_T CreatePNG(STRING_T strFilePath,STRING_T strFileName,STRING_T strPngPath)
{
    STRING_T strParamter = STRING_T();
    STRING_T strVideo = strFilePath + strFileName;
    STRING_T strPng = strPngPath + strFileName;
    UINT32_T ui32Len = strPng.length();

    //调用opencv截取视频图像
    VideoCapture capture(strVideo.c_str());
    if (!capture.isOpened())
    {
        return EXIT_ERR;
    }

    FLOATU_T fRate = capture.get(CV_CAP_PROP_FPS);    //帧率
    FLOATU_T fTotal = capture.get(CV_CAP_PROP_FRAME_COUNT);  //总帧数
    INT32_T  delay = 0;

    if (fTotal >= fRate)
    {
        if ((INT32_T)fTotal/(INT32_T)fRate >= 5)
        {
            delay = (INT32_T)(fRate);
        } 
        else
        {
           delay = (INT32_T)fTotal/5; 
        }
    }
    else
    {
        if ((INT32_T)fTotal >= 5)
        {
           delay = (INT32_T)fTotal/5;
        } 
        else
        {
           delay = 1; 
        }
    }

    if (delay == 0)
    {
        capture.release();
        return EXIT_ERR;
    }

    cout<<fRate<<" "<<fTotal<<" "<<delay<<endl;

    BOOL_T   bStop = BFALSE;
    Mat      frame;
    INT32_T count = 0;
    strPng = strPng.erase(ui32Len-4,4);  //去掉.mp4后缀
    INT32_T  i32Loop = 0;

    while(!bStop)
    {
        for (INT32_T i32FrameNum = 0;i32FrameNum < delay;i32FrameNum++)
        {
            capture.read(frame);
            i32Loop++;

            if (i32Loop >= ((INT32_T)fTotal - 1) || (frame.cols == 0 && frame.rows == 0))
            {
                bStop = BTRUE;
                break;
            }
        }

        if (!bStop && frame.cols !=0 && frame.rows != 0)
        {
            strParamter = strPng + Int32ToString(count) + ".png";
            imwrite(strParamter,frame);
            count++;
        }

        if (count == 5)
        {
            bStop = BTRUE;
        }
    }
    cout<<"count "<<count<<endl;
    capture.release();
    //结束视频图像截取
    return EXIT_OK;
}

/** 判断图片是否是黑屏及蓝屏*/
static INT32_T JudgeBlackPng(STRING_T strPngPath,StringList_V &vFileName)
{
    INT32_T      i32Ret = EXIT_OK;
    INT32_T      i32FileNum = 0;
    INT32_T      i32BlackFile = 0;
    HistogramND  hist;
    Mat          image;

    i32FileNum = vFileName.size();
    if ( 0 == i32FileNum)
    {
        return EXIT_ERR;
    }

    for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum; i32Index++)
    {
        STRING_T path = strPngPath + vFileName[i32Index]; 
        if (!hist.importImage(path))
        {  
            cout << "Import Error!" << endl;
            continue;  
        }

        hist.getHistogram();

        image = hist.displayHisttogram();
        if (hist.i32Area < 800)
        {
            i32BlackFile++;
        }
        //namedWindow("Hist");
        //imshow("Hist",image);
        //waitKey();

        DeleteFile(path.c_str());
    }

    if (i32BlackFile > 2)
    {
        i32Ret = EXIT_ERR;
    }
    return i32Ret;
}

//转换png的RGB图像到Gray图像，并按每30个像素的间隔切割图像
static INT32_T ChangeRGB2GRAY(STRING_T strPngPath,StringList_V &vFileName)
{
    INT32_T      i32Ret = EXIT_OK;
    INT32_T      i32Rows = 0;
    INT32_T      i32Cols = 0;
    INT32_T      i32FileNum = 0;
    cv::Mat      org,dst,img,tmp;
    STRING_T     tmp_path,tmp_flag;

    i32FileNum = vFileName.size();
    if ( 0 == i32FileNum)
    {
        cout<<"i32FileNum == 0 "<<endl;
        return EXIT_ERR;
    }

    for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum; i32Index++)
    {
        STRING_T path = strPngPath + vFileName[i32Index];
        org = imread(path);           //直接读取成彩色图

        cvtColor(org,dst,CV_BGR2GRAY);//处理成单通道的灰度图
#if 0
        //图像不做切割，保留原始大小
        tmp_path = strPngPath + "gray\\" + vFileName[i32Index];
        imwrite(tmp_path,dst);
#else   //图像切割
        i32Cols = dst.cols;   //高度
        i32Rows = dst.rows;   //宽度
        for (INT32_T i32ColIndex = INDEXBEGIN;i32ColIndex < (i32Cols-280);i32ColIndex += 30)
        {
            if (i32ColIndex > i32Cols - 280)
            {
                i32ColIndex = i32Cols - 280;
            }

            for (INT32_T i32RowIndex = INDEXBEGIN;i32RowIndex < (i32Rows-220);i32RowIndex += 30)
            {
                if (i32RowIndex > i32Rows - 220)
                {
                    i32RowIndex = i32Rows - 220;
                }

                if (i32ColIndex != i32RowIndex)  //过滤掉一部分
                {
                    continue;
                }

                tmp = dst(Rect(i32ColIndex,i32RowIndex,280,220));
                tmp_path = strPngPath + "gray\\" + vFileName[i32Index];
                tmp_flag = Int32ToString(i32ColIndex) + Int32ToString(i32RowIndex) + ".png";
                tmp_path.replace(tmp_path.length()-4,4,tmp_flag.c_str());
                imwrite(tmp_path,tmp);
            }
        }
#endif

        ::DeleteFile(path.c_str());
        i32Ret = EXIT_OK;
    }

    return i32Ret;
}

//转换png的RGB图像到Gray图像，并保持原图大小
static INT32_T ChangeRGB2GRAYNormal(STRING_T strPngPath,StringList_V &vFileName)
{
    INT32_T      i32Ret = EXIT_OK;
    INT32_T      i32FileNum = 0;
    cv::Mat      org,dst;

    i32FileNum = vFileName.size();
    if ( 0 == i32FileNum)
    {
        return EXIT_ERR;
    }

    for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum; i32Index++)
    {
        STRING_T path = strPngPath + vFileName[i32Index];

        org = imread(path);
        cvtColor(org,dst,CV_BGR2GRAY);
        ::DeleteFile(path.c_str());
        imwrite(path,dst);
        i32Ret = EXIT_OK;
    }

    return i32Ret;
}


//原始图像切割，按照指定规格
static INT32_T CutPicture(STRING_T strPngPath,StringList_V &vFileName,UINT32_T ui32CutCols,
    UINT32_T ui32CutRows,UINT32_T ui32Move)
{
    INT32_T      i32Ret = EXIT_OK;
    INT32_T      i32Rows = 0;
    INT32_T      i32Cols = 0;
    INT32_T      i32FileNum = 0;
    cv::Mat      org,dst,img,tmp;
    STRING_T     tmp_path,tmp_flag;

    i32FileNum = vFileName.size();
    if ( 0 == i32FileNum)
    {
        cout<<"i32FileNum == 0 "<<endl;
        return EXIT_ERR;
    }

    for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum; i32Index++)
    {
        STRING_T path = strPngPath + vFileName[i32Index];
        org = imread(path);   //直接读取成彩色图

        //图像切割
        i32Cols = org.cols;   //宽度
        i32Rows = org.rows;   //高度
        for (INT32_T i32ColIndex = INDEXBEGIN;i32ColIndex < (i32Cols-ui32CutCols);i32ColIndex += ui32Move)
        {
            if (i32ColIndex > i32Cols - ui32CutCols)
            {
                i32ColIndex = i32Cols - ui32CutCols;
            }

            for (INT32_T i32RowIndex = INDEXBEGIN;i32RowIndex < (i32Rows-ui32CutRows);i32RowIndex += ui32Move)
            {
                if (i32RowIndex > i32Rows - ui32CutRows)
                {
                    i32RowIndex = i32Rows - ui32CutRows;
                }

                tmp = org(Rect(i32ColIndex,i32RowIndex,ui32CutCols,ui32CutRows));
                tmp_path = strPngPath + "gray\\" + vFileName[i32Index];
                tmp_flag = Int32ToString(i32ColIndex) + Int32ToString(i32RowIndex) + ".png";
                tmp_path.replace(tmp_path.length()-4,4,tmp_flag.c_str());
                imwrite(tmp_path,tmp);
            }
        }
        ::DeleteFile(path.c_str());
        i32Ret = EXIT_OK;
        cout<<"Cut Picture: "<<i32Index+1<<" ,Total: "<<i32FileNum<<endl;
    }
    cout<<"All Picture Cuted!!!"<<endl;
    return i32Ret;
}


//打开PNG文件，准备训练数据
static INT32_T OpenGrayPNG(NNTarget_V *out_pvTarget, NNTrain_V *out_pvTrain, UINT32_T *out_pui32Num,\
    UINT32_T *out_pui32Each,UINT32_T ui32VideoFlag, STRING_T strFilePath, StringList_V &vFileName,\
    INT32_T i32cols,INT32_T i32rows,FLOATU_T fTargetRate,INT32_T i32TargetNum)
{
    UINT32_T     ui32FileNum;
    STRING_T     strTmpFile = STRING_T();
    STRING_T     strBakFile = STRING_T();
    FileInfo_S   sFileInfo;
    PADDRESS_T   pOffset;
    LENGTH_T     ltReadSize, ltTotal;
    NNTarget_S   *psTarget;
    NNTrain_S    *psTrain;
    UINT16_T     ui16MaxPixel = 0;
    UCHAR_T      uchPixel;
    FLOATU_T     fPixel = 0.0;
    FLOATU_T     *pf64In;
    FLOATU_T     af64RRRIn[500 * sizeof(FLOATU_T)] = {0};
    CFileBase    *pcFile = new CFileBase();
    PADDRESS_T   pFileBuf = CMemoryBase::MemoryMalloc(10*1024*1024);
    PADDRESS_T   pOutBuf = CMemoryBase::MemoryMalloc(10*1024*1024);
    PADDRESS_T   pTmp = NULL;
    STRING_T     strLog;

    ui32FileNum = vFileName.size();
    *out_pui32Num = ui32FileNum;                        //图片总数
    *out_pui32Each = i32cols*i32rows;                   //每张图片像素值，为灰度图

    for(UINT32_T ui32Index = INDEXBEGIN; ui32Index < ui32FileNum; ui32Index++)
    {
        memset((UCHAR_T*)pFileBuf,0,10*1024*1024);
        memset((UCHAR_T*)pOutBuf,0,10*1024*1024);
        ui16MaxPixel = 0;

        strTmpFile = strFilePath + vFileName[ui32Index];
        sFileInfo.fptFilePath = strTmpFile;
        sFileInfo.strOpenFileMode = "rb";

        //读文件并解码转换
        pcFile->SetFileEnv(&sFileInfo);
        pOffset = pFileBuf;
        ltTotal = 0;
        while (BTRUE)
        {
            ltReadSize = pcFile->ReadBySize(4096, (UCHAR_T *)pOffset);
            if (0 == ltReadSize)
            {
                break;
            }
            pOffset += ltReadSize;
            ltTotal += ltReadSize;
        }
        pcFile->ResetFileEnv();

        PNGDecoder::Decode(pOutBuf, &ltReadSize, pFileBuf, ltTotal);
        pTmp = pOutBuf;
        for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < i32cols*i32rows; ui16Index++)
        {
            uchPixel = *(UCHAR_T*)pTmp;
            if (uchPixel > ui16MaxPixel)
            {
                ui16MaxPixel = uchPixel;
            }
            //strLog += "|" + FloatToString((FLOATU_T)uchPixel/255.0);
            pTmp += sizeof(UCHAR_T);
        }
        ui16MaxPixel = (ui16MaxPixel > 255)?65535:255;     //查找最大像素值

        psTarget = new NNTarget_S(i32TargetNum);
        psTarget->ui32Size = i32TargetNum;
        for (UINT16_T ui16Index = INDEXBEGIN;ui16Index < i32TargetNum;ui16Index++)
        {
            if (ui16Index == ui32VideoFlag)
            {
                psTarget->pfTarget[ui16Index] = fTargetRate;
            }
            else
            {
                psTarget->pfTarget[ui16Index] = 1-fTargetRate; 
            }
        }
        out_pvTarget->push_back(psTarget);

        psTrain = new NNTrain_S(i32rows,i32cols);
        psTrain->i32ScaleH = i32rows;
        psTrain->i32ScaleW = i32cols;
        pf64In = (FLOATU_T *)psTrain->apData[0];

        pTmp = pOutBuf;
        for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < i32rows; ui16Index++)
        {
            for(UINT16_T ui16Index2 = INDEXBEGIN;ui16Index2 < i32cols;ui16Index2++)
            {
                uchPixel = *(UCHAR_T*)pTmp;
                fPixel = (FLOATU_T)uchPixel/ui16MaxPixel;
                SystemTrans::Memcpy(&af64RRRIn[ui16Index2],&fPixel,sizeof(FLOATU_T));   //存灰度图像素值
                pTmp += sizeof(UCHAR_T);
            }

            //1行数据结束
            SystemTrans::Memcpy(pf64In + ui16Index*i32cols,af64RRRIn,i32cols*sizeof(FLOATU_T));
            memset(af64RRRIn,0,sizeof(FLOATU_T)*i32cols);
        }
        out_pvTrain->push_back(psTrain); 

        //测试
        //strLog = STRING_T();
        //pTmp = (PADDRESS_T)pf64In;
        //for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < i32cols*i32rows; ui16Index++)
        //{
        //    fPixel = *(FLOATU_T*)pTmp;
        //    strLog += "|" + FloatToString(fPixel);
        //    pTmp += sizeof(FLOATU_T);
        //}
        //测试结束

        strBakFile = sFileInfo.fptFilePath.replace(strFilePath.length()-5,4,"gray_bak");
        CopyFile(strTmpFile.c_str(),strBakFile.c_str(),FALSE);
    }

    CMemoryBase::MemoryFree(pFileBuf);
    CMemoryBase::MemoryFree(pOutBuf);
    delete pcFile;
    return EXIT_OK;
}

//打开PNG文件，准备训练数据
static INT32_T OpenPNG(NNTarget_V *out_pvTarget, NNTrain_V *out_pvTrain, UINT32_T *out_pui32Num,\
 UINT32_T *out_pui32Each,UINT32_T ui32VideoFlag, STRING_T strFilePath, StringList_V &vFileName,\
 INT32_T i32cols,INT32_T i32rows,FLOATU_T fTargetRate,INT32_T i32TargetNum)
{
    UINT32_T     ui32FileNum;
    STRING_T     strTmpFile = STRING_T();
    STRING_T     strBakFile = STRING_T();
    FileInfo_S   sFileInfo;
    PADDRESS_T   pOffset;
    LENGTH_T     ltReadSize, ltTotal;
    NNTarget_S   *psTarget;
    NNTrain_S    *psTrain;
    UINT16_T     ui16MaxPixel = 0;
    UCHAR_T      uchPixel;
    FLOATU_T     fPixel = 0.0;
    FLOATU_T     *pf64In;
    FLOATU_T     af64In[500 * sizeof(FLOATU_T)] = {0};
    FLOATU_T     af64RGBIn[CHANNEL_NUM * sizeof(FLOATU_T)] = {0};
    FLOATU_T     af64RRRIn[500 * sizeof(FLOATU_T)] = {0};
    FLOATU_T     af64RIn[500 * sizeof(FLOATU_T)] = {0};
    FLOATU_T     af64GIn[500 * sizeof(FLOATU_T)] = {0};
    FLOATU_T     af64BIn[500 * sizeof(FLOATU_T)] = {0};
    CFileBase    *pcFile = new CFileBase();
    PADDRESS_T   pFileBuf = CMemoryBase::MemoryMalloc(10*1024*1024);
    PADDRESS_T   pOutBuf = CMemoryBase::MemoryMalloc(10*1024*1024);
    PADDRESS_T   pTmp = NULL;
    STRING_T     strLog;

    ui32FileNum = vFileName.size();
    *out_pui32Num = ui32FileNum;                        //图片总数
    *out_pui32Each = i32cols*i32rows*CHANNEL_NUM;       //每张图片像素值，为彩图

    for(UINT32_T ui32Index = INDEXBEGIN; ui32Index < ui32FileNum; ui32Index++)
    {
        memset((UCHAR_T*)pFileBuf,0,10*1024*1024);
        memset((UCHAR_T*)pOutBuf,0,10*1024*1024);
        ui16MaxPixel = 0;

        strTmpFile = strFilePath + vFileName[ui32Index];
        sFileInfo.fptFilePath = strTmpFile;
        sFileInfo.strOpenFileMode = "rb";

        //读文件并解码转换
        pcFile->SetFileEnv(&sFileInfo);
        pOffset = pFileBuf;
        ltTotal = 0;
        while (BTRUE)
        {
            ltReadSize = pcFile->ReadBySize(4096, (UCHAR_T *)pOffset);
            if (0 == ltReadSize)
            {
                break;
            }
            pOffset += ltReadSize;
            ltTotal += ltReadSize;
        }
        pcFile->ResetFileEnv();

        PNGDecoder::Decode(pOutBuf, &ltReadSize, pFileBuf, ltTotal);
        pTmp = pOutBuf;
        for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < i32cols*i32rows*CHANNEL_NUM; ui16Index++)
        {
            uchPixel = *(UCHAR_T*)pTmp;
            if (uchPixel > ui16MaxPixel)
            {
                ui16MaxPixel = uchPixel;
            }
            //strLog += "|" + FloatToString((FLOATU_T)uchPixel/255.0);
            pTmp += sizeof(UCHAR_T);
        }
        ui16MaxPixel = (ui16MaxPixel > 255)?65535:255;     //查找最大像素值

        psTarget = new NNTarget_S(i32TargetNum);
        psTarget->ui32Size = i32TargetNum;
        for (UINT16_T ui16Index = INDEXBEGIN;ui16Index < i32TargetNum;ui16Index++)
        {
            if (ui16Index == ui32VideoFlag)
            {
                psTarget->pfTarget[ui16Index] = fTargetRate;
            }
            else
            {
                psTarget->pfTarget[ui16Index] = 1-fTargetRate; 
            }
        }
        out_pvTarget->push_back(psTarget);

        //在这里按照RRRRR..../GGGGG..../BBBBB...的顺序排列数据
        psTrain = new NNTrain_S(i32rows,i32cols*CHANNEL_NUM);
        psTrain->i32ScaleH = i32rows;
        psTrain->i32ScaleW = i32cols*CHANNEL_NUM;
        pf64In = (FLOATU_T *)psTrain->apData[0];

#ifdef  RRR_DATA
        for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < CHANNEL_NUM; ui16Index++)
        {
            pTmp = pOutBuf + ui16Index*sizeof(UCHAR_T);
            for (UINT16_T ui16Index1 = INDEXBEGIN; ui16Index1 < i32rows; ui16Index1++) //行
            {
                for(UINT16_T ui16Index2 = INDEXBEGIN;ui16Index2 < i32cols;ui16Index2++) //列
                {
                    uchPixel = *(UCHAR_T*)pTmp;
                    fPixel = (FLOATU_T)uchPixel/ui16MaxPixel;
                    //存RRRR.../GGGG.../BBBB...数据
                    SystemTrans::Memcpy(&af64RRRIn[ui16Index2],&fPixel,sizeof(FLOATU_T));
                    pTmp += CHANNEL_NUM*sizeof(UCHAR_T);
                }

                //1行数据结束
                SystemTrans::Memcpy(pf64In + (ui16Index*i32rows + ui16Index1)*i32cols,\
                    af64RRRIn,i32cols*sizeof(FLOATU_T));
                memset(af64RRRIn,0,500*sizeof(FLOATU_T));
            }
        }

#elif  RGB_DATA
        pTmp = pOutBuf;
        for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < i32rows; ui16Index++)
        {
            for(UINT16_T ui16Index2 = INDEXBEGIN;ui16Index2 < i32cols;ui16Index2++)
            {
                for (UINT16_T ui16Index3 = INDEXBEGIN;ui16Index3 < CHANNEL_NUM;ui16Index3++)
                {
                    uchPixel = *(UCHAR_T*)pTmp;
                    fPixel = (FLOATU_T)uchPixel/ui16MaxPixel;
                    SystemTrans::Memcpy(&af64RGBIn[ui16Index3],&fPixel,sizeof(FLOATU_T));   //存RGB数据
                    pTmp += sizeof(UCHAR_T);
                }
                SystemTrans::Memcpy(&af64In[ui16Index2*CHANNEL_NUM],af64RGBIn,CHANNEL_NUM*sizeof(FLOATU_T));
                memset(af64RGBIn,0,sizeof(FLOATU_T)*CHANNEL_NUM);
            }

            //1行数据结束
            SystemTrans::Memcpy(pf64In + ui16Index*i32cols*CHANNEL_NUM,af64In,\
                CHANNEL_NUM*i32cols*sizeof(FLOATU_T));
            memset(af64In,0,i32cols*sizeof(FLOATU_T)*CHANNEL_NUM);
        }
#else
        //保存数据每一行按照 rrrrrggggggbbbbbb格式，有多少行就有多少组
        pTmp = pOutBuf;
        for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < i32rows; ui16Index++)  //行数
        {
            for(UINT16_T ui16Index2 = INDEXBEGIN;ui16Index2 < i32cols;ui16Index2++) //列数
            {
                for (UINT16_T ui16Index3 = INDEXBEGIN;ui16Index3 < CHANNEL_NUM;ui16Index3++)
                {
                    uchPixel = *(UCHAR_T*)pTmp;
                    fPixel = (FLOATU_T)uchPixel/ui16MaxPixel;

                    if (ui16Index3 == 0)
                    {
                        SystemTrans::Memcpy(&af64RIn[ui16Index2],&fPixel,sizeof(FLOATU_T));
                    } 
                    else if (ui16Index3 == 1)
                    {
                        SystemTrans::Memcpy(&af64GIn[ui16Index2],&fPixel,sizeof(FLOATU_T));
                    }
                    else
                    {
                        SystemTrans::Memcpy(&af64BIn[ui16Index2],&fPixel,sizeof(FLOATU_T));
                    }
                    pTmp += sizeof(UCHAR_T);
                }
            }

            //1行数据结束
            SystemTrans::Memcpy(pf64In+ui16Index*CHANNEL_NUM*i32cols,af64RIn,i32cols*sizeof(FLOATU_T));
            SystemTrans::Memcpy(pf64In+(ui16Index*CHANNEL_NUM+1)*i32cols,af64GIn,i32cols*sizeof(FLOATU_T));
            SystemTrans::Memcpy(pf64In+(ui16Index*CHANNEL_NUM+2)*i32cols,af64BIn,i32cols*sizeof(FLOATU_T));

            memset(af64RIn,0,500*sizeof(FLOATU_T));
            memset(af64GIn,0,500*sizeof(FLOATU_T));
            memset(af64BIn,0,500*sizeof(FLOATU_T));
        }
#endif
        out_pvTrain->push_back(psTrain); 

        //测试
        //strLog = STRING_T();
        //pTmp = (PADDRESS_T)pf64In;
        //for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < i32cols*i32rows*CHANNEL_NUM; ui16Index++)
        //{
        //    fPixel = *(FLOATU_T*)pTmp;
        //    strLog += "|" + FloatToString(fPixel);
        //    pTmp += sizeof(FLOATU_T);
        //}
        //测试结束

        strBakFile = sFileInfo.fptFilePath.replace(strFilePath.length()-5,4,"gray_bak");
        CopyFile(strTmpFile.c_str(),strBakFile.c_str(),FALSE);
    }

    CMemoryBase::MemoryFree(pFileBuf);
    CMemoryBase::MemoryFree(pOutBuf);
    delete pcFile;
    return EXIT_OK;
}

VOID_T CVideoOpenCV::InitData(ConfigStruct_S *pcConfig,CLogBase *pcLog)
{
    m_strFilePath = pcConfig->strVideoFilePath;
    m_strNetConfig1 = pcConfig->strNetConfig1;
    m_strNetConfig2 = pcConfig->strNetConfig2;
    m_strNetConfig3 = pcConfig->strNetConfig3;
    m_strNetFilePath1 = pcConfig->strNetFilePath1;
    m_strNetFilePath2 = pcConfig->strNetFilePath2;
    m_strNetFilePath3 = pcConfig->strNetFilePath3;
    m_strPngFile = pcConfig->strPngFile;
    m_strResultPath = pcConfig->strResultPath;
    m_ui32VideoFlag = pcConfig->ui32VideoFlag;
    m_pcLog = pcLog;
    m_i32cols = pcConfig->i32cols;
    m_i32rows = pcConfig->i32rows;
    m_i32Move = pcConfig->i32Move;
    m_i32NetNum = pcConfig->i32NetNum;
    m_f64Err = pcConfig->f64Err;
    m_fTargetRate = pcConfig->fTarget;
    m_i32TargetNum = pcConfig->i32TargetNum;
    m_fResultRate = pcConfig->fResultRate;
}

INT32_T CVideoOpenCV::StartWork()
{
    ThreadInitInfo_S sInfo;
    STRING_T         strName;
    BOOL_T           bIsTool = 0;

    if (bIsTool)
    {
        //sInfo.pFunAddr = (THREADFUN)VideoToolThread;  //人工截图
        sInfo.pFunAddr = (THREADFUN)VideoBlackThread;   //黑屏判断
        strName = "VideoTool";
        RegisterThread(strName,&sInfo);
        Start(strName);
    }
    else
    {
        //sInfo.pFunAddr = (THREADFUN)VideoOpenCVThread;  //图像压缩
        sInfo.pFunAddr = (THREADFUN)VideoNetFileAnaly;  //视频判断
        //sInfo.pFunAddr = (THREADFUN)PicutreCutThread;     //图片剪切
        strName = "VideoOpenCV";
        RegisterThread(strName,&sInfo);
        Start(strName); 
    }

    return EXIT_OK;
}

INT32_T CVideoOpenCV::StopWork()
{
    STRING_T strName = "VideoOpenCV";
    Terminate(strName);
    JoinThread(strName);
    UnregisterThread(strName);

    return EXIT_OK;
}

/** 视频处理工具线程*/
UINT32_T ThreadSpace::VideoToolThread(PVOID_T psParam)
{
    CVideoOpenCV *pcVideo = (CVideoOpenCV*)((ThreadParameter_S*)psParam)->pThreadPool;
    STRING_T     strFilePath = pcVideo->m_strFilePath;
    STRING_T     strNetFilePath = pcVideo->m_strNetFilePath1;
    STRING_T     strPngFilePath = pcVideo->m_strPngFile;
    STRING_T     strResultPath = pcVideo->m_strResultPath;
    UINT32_T     ui32VideoFlag = pcVideo->m_ui32VideoFlag;
    StringList_V vFileName,vTmp;
    INT32_T      i32FileNum = 0,i32NameLen = 0,i32Ret = EXIT_OK;
    STRING_T     strComplete = STRING_T(),strTmpFileName = STRING_T(),strFileSuffix = STRING_T();           
    UserData_S   UserDataObj;
    STRING_T     strLog;

    getJustCurrentFile(strFilePath,vFileName);
    i32FileNum = vFileName.size();

    while(BTRUE)
    {
#if 0
        //将png图片RGB格式转换成灰度图
        for (UINT32_T ui32Index = INDEXBEGIN;ui32Index < i32FileNum; ui32Index++)
        {
            STRING_T  path = strFilePath + vFileName[ui32Index];
            IplImage* porg = cvLoadImage(path.c_str(),1);
            IplImage* dst = cvCreateImage(cvGetSize(porg),porg->depth,1);
            cvCvtColor(porg,dst,CV_BGR2GRAY);

            STRING_T tmp_path = path;
            tmp_path.replace(14,3,"gray");
            cvSaveImage(tmp_path.c_str(),dst);
        }
        break;
        //结束转换

        //删除大量的黑屏图片
        for (UINT32_T ui32Index = INDEXBEGIN;ui32Index < i32FileNum; ui32Index++)
        {
            string path = strFilePath + vFileName[ui32Index];
            HistogramND hist;  
            if (!hist.importImage(path))
            {  
                cout << "Import Error!" << endl;
                continue;  
            }  
            hist.splitChannels();  
            i32Ret = hist.getHistogram();
            if (i32Ret != EXIT_OK)
            {
                string tmppath = path;
                i32NameLen = tmppath.length();
                tmppath.replace(14,3,"err");
                CopyFile(path.c_str(),tmppath.c_str(),FALSE);
                DeleteFile(path.c_str());
                cout<<"DeleteFile: "<<path.c_str()<<endl;
            } 
            //hist.displayHisttogram();  
            //waitKey();
        }
        break;
        //结束黑屏图片删除
#endif

        //将jpg图片转换成png图片
        for (UINT32_T ui32Index = INDEXBEGIN; ui32Index < i32FileNum;ui32Index++)
        {
            strComplete = strFilePath + vFileName[ui32Index];
            UserDataObj.strName = strComplete;
            org = imread(strComplete);
            org.copyTo(img);
            org.copyTo(tmp);
            gPos = 100;

            namedWindow("img");
            setMouseCallback("img",on_mouse,&UserDataObj);
            imshow("img",img);
            waitKey();

            string tmppath = strComplete;
            i32NameLen = tmppath.length();

            if (gPos == 0)
            {
                tmppath.replace(14,4,"bak\\all_bak\\");
            }
            else
            {
                tmppath.replace(14,4,"bak\\all_other1_bak\\"); 
            }
            CopyFile(strComplete.c_str(),tmppath.c_str(),FALSE);
            DeleteFile(strComplete.c_str());
        }
        break;
        //结束转换
    }
    pcVideo->SetThreadEnd(((ThreadParameter_S*)psParam)->strThreadName);
    return EXIT_OK;
}


/*初始化网络文件*/
static INT32_T InitNetFile(CConfigText *pcConfig,CCNN *pcANN,CNNScale_S *psScale,NNTarget_V  *pvTarget,
    NNTrain_V  *pvTrain,STRING_T  strNetFilePath,STRING_T strConfigFile)
{
    STRING_T strTmp,strNum;

    pcConfig->ReadConfig(strConfigFile);
    memset(psScale,0,sizeof(CNNScale_S));
    psScale->sInput.i32Height = StringToInt32(pcConfig->GetParameter("InputH"));
    psScale->sInput.i32Width = StringToInt32(pcConfig->GetParameter("InputW"));
    psScale->ui32LayerNum = StringToInt32(pcConfig->GetParameter("LayerNum"));
    psScale->ui32RecordRate = StringToInt32(pcConfig->GetParameter("RecordRate"));
    psScale->ui32GradNum = StringToInt32(pcConfig->GetParameter("GradNum"));
    psScale->fEta = (FLOATU_T)StringToFloat(pcConfig->GetParameter("Eta"));
    psScale->fMomentum = (FLOATU_T)StringToFloat(pcConfig->GetParameter("Momentum"));

    for (UINT16_T ui16Index = INDEXBEGIN; ui16Index < psScale->ui32LayerNum; ui16Index++)
    {
        strNum = Int32ToString(ui16Index);
        strTmp = pcConfig->GetParameter("Type" + strNum);
        if ("Conv" == strTmp) 
            psScale->aeType[ui16Index] = LAYER_CONV;
        else if ("Pool" == strTmp) 
            psScale->aeType[ui16Index] = LAYER_POOL;
        else if ("FC" == strTmp) 
            psScale->aeType[ui16Index] = LAYER_FC;
        else 
            psScale->aeType[ui16Index] = LAYER_OUTPUT;

        strTmp = pcConfig->GetParameter("ACType" + strNum);
        if ("ReLu" == strTmp) 
            psScale->aeACFunType[ui16Index] = ACFUN_RELU;
        else if ("Sigmoid" == strTmp) 
            psScale->aeACFunType[ui16Index] = ACFUN_SIGMOID;
        else if ("Tanh" == strTmp) 
            psScale->aeACFunType[ui16Index] = ACFUN_TANH;
        else 
            psScale->aeACFunType[ui16Index] = ACFUN_DEFAULT;

        strTmp = pcConfig->GetParameter("ConvType" + strNum);
        if ("Valid" == strTmp) 
            psScale->aeConvType[ui16Index] = CONV_VALID;
        else if ("Same" == strTmp) 
            psScale->aeConvType[ui16Index] = CONV_SAME;
        else 
            psScale->aeConvType[ui16Index] = CONV_DEFAULT;

        strTmp = pcConfig->GetParameter("PoolType" + strNum);
        if ("Max" == strTmp) 
            psScale->aePoolType[ui16Index] = POOLING_MAX;
        else if ("Avg" == strTmp) 
            psScale->aePoolType[ui16Index] = POOLING_AVG;
        else 
            psScale->aePoolType[ui16Index] = POOLING_DEFAULT;

        psScale->ai32Stride[ui16Index] = StringToInt32(pcConfig->GetParameter("Stride" + strNum));
        psScale->aui32UnitNum[ui16Index] = StringToInt32(pcConfig->GetParameter("UnitNum" + strNum));
        psScale->asWeightSize[ui16Index].i32Height = StringToInt32(pcConfig->GetParameter("WeightH" + strNum));
        psScale->asWeightSize[ui16Index].i32Width = StringToInt32(pcConfig->GetParameter("WeightW" + strNum));
    }

    pcANN->Init(1,pvTrain,pvTarget,psScale,strNetFilePath);
    pcANN->StartWork();
    return EXIT_OK;
}

//进行网络数据判断
static INT32_T AnalyzeNetData(CCNN *pcANN,NNTarget_V *pvTarget,NNTrain_V *pvTrain,UINT32_T ui32VideoFlag,
    STRING_T strPngFilePath,STRING_T strPngFilePath2,StringList_V &vPngFileName,
    CVideoOpenCV *pcVideo,vector<UINT32_T> *pvInt,INT32_T i32Flag,FLOATU_T f64Err, INT32_T i32cols,\
    INT32_T i32rows,FLOATU_T fTargetRate,INT32_T i32TargetNum)
{
    UINT32_T   ui32TrainNum = 0,ui32Each = 0;
    NNTest_S   *psTest = NULL;
    NNTarget_S *psTarget = NULL;
    NNTrain_S  *psTrain = NULL;
    STRING_T   strLog;
    
#if 1
    OpenGrayPNG(pvTarget,pvTrain,&ui32TrainNum,&ui32Each,ui32VideoFlag,strPngFilePath,vPngFileName,\
        i32cols,i32rows,fTargetRate,i32TargetNum);
#else
    OpenPNG(pvTarget,pvTrain,&ui32TrainNum,&ui32Each,ui32VideoFlag,strPngFilePath,vPngFileName,\
        i32cols,i32rows,fTargetRate,i32TargetNum);
#endif

    psTest = new NNTest_S(ui32TrainNum,i32TargetNum);
    if (EXIT_OK != pcANN->Test(psTest,pvTrain,pvTarget,f64Err,BTRUE))
    {
        pvTarget->clear();
        pvTrain->clear();
        delete psTest;
        return EXIT_ERR;
    }

    pvInt->clear();
    for (UINT32_T ui32FileIndex = INDEXBEGIN;ui32FileIndex < psTest->ui32Num;ui32FileIndex++)
    {
        NNTarget_S *psTestTarget = (*psTest->pvTarget)[ui32FileIndex];
        NNTarget_S *psTarget = (*pvTarget)[ui32FileIndex];
        BOOL_T    bIsRight = BTRUE;

        for(INT32_T i32Index = INDEXBEGIN;i32Index < psTest->ui32Size;i32Index++)
        {
            FLOATU_T f64TestSD = psTestTarget->pfTarget[i32Index];
            FLOATU_T f64SD = psTarget->pfTarget[i32Index];

            if (abs(f64SD - f64TestSD) > f64Err)
            {
                bIsRight = BFALSE;
                break;
            }
        }

        if (bIsRight)
        {
            pvInt->push_back(ui32FileIndex);
        }
    }

    for(UINT32_T ui32RightIndex = INDEXBEGIN;ui32RightIndex < pvInt->size();ui32RightIndex++)
    {
        STRING_T strRightFile = strPngFilePath + vPngFileName[(*pvInt)[ui32RightIndex]];
        STRING_T strRightFile2 = strPngFilePath2 + vPngFileName[(*pvInt)[ui32RightIndex]];

        CopyFile(strRightFile.c_str(),strRightFile2.c_str(),BFALSE);
    }

    for (UINT32_T ui32DeleteIndex = INDEXBEGIN;ui32DeleteIndex < vPngFileName.size();ui32DeleteIndex++)
    {
        STRING_T strDeleteFile = strPngFilePath + vPngFileName[ui32DeleteIndex];
        DeleteFile(strDeleteFile.c_str());
    }

    for (UINT32_T ui32Index = INDEXBEGIN;ui32Index < pvTarget->size();++ui32Index)
    {
        psTarget = (*pvTarget)[ui32Index];
        delete psTarget;
        psTrain = (*pvTrain)[ui32Index];
        delete psTrain;
    }
    pvTarget->clear();
    pvTrain->clear();
    delete psTest;
    return EXIT_OK;
}

/** 黑屏、蓝屏视频分析线程*/
UINT32_T ThreadSpace::VideoBlackThread(PVOID_T psParam)
{
    CVideoOpenCV *pcVideo = (CVideoOpenCV*)((ThreadParameter_S*)psParam)->pThreadPool;
    STRING_T     strFilePath = pcVideo->m_strFilePath;
    STRING_T     strPngFilePath = pcVideo->m_strPngFile;
    STRING_T     strResultPath  = pcVideo->m_strResultPath;
    UINT32_T     ui32VideoFlag  = pcVideo->m_ui32VideoFlag;
    StringList_V vFileName,vPngFileName;
    INT32_T      i32FileNum = 0,i32Ret = EXIT_OK;
    STRING_T     strSrc,strDst;

    getJustCurrentFile(strFilePath,vFileName);
    i32FileNum = vFileName.size();

    while(BTRUE)
    {
        //从视频中获取png文件
        pcVideo->m_pcLog->LogToShell(LOG_INFO,"Start Video Analyze ....",BTRUE);
        for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum;i32Index++)
        {
            pcVideo->m_pcLog->LogToFile(LOG_INFO,"Start " + Int32ToString(i32Index+1)\
                + " Video Analyze ....");
            if (EXIT_OK != CreatePNG(strFilePath,vFileName[i32Index],strPngFilePath))
            {
                pcVideo->m_pcLog->LogToShell(LOG_ERROR,"CreatePNG Error",BTRUE);
                continue;
            }

            vPngFileName.clear();
            getJustCurrentFile(strPngFilePath,vPngFileName);

            if (EXIT_OK != JudgeBlackPng(strPngFilePath,vPngFileName))
            {
                strSrc = strFilePath + vFileName[i32Index];
                strDst = strResultPath + vFileName[i32Index];
                CopyFile(strSrc.c_str(),strDst.c_str(),BFALSE);
                DeleteFile(strSrc.c_str());
            }

            pcVideo->m_pcLog->LogToFile(LOG_INFO,"Completed " + Int32ToString(i32Index+1)\
                + " Video Analyze ....");
        }
        pcVideo->m_pcLog->LogToShell(LOG_INFO,"Completed All Video Analyze ....",BTRUE);
        break;
    }

    pcVideo->SetThreadEnd(((ThreadParameter_S*)psParam)->strThreadName);
    return EXIT_OK;
}

/** 视频网络文件分析线程*/
UINT32_T ThreadSpace::VideoNetFileAnaly(PVOID_T psParam)
{
    CVideoOpenCV *pcVideo = (CVideoOpenCV*)((ThreadParameter_S*)psParam)->pThreadPool;
    STRING_T     strFilePath = pcVideo->m_strFilePath;
    INT32_T      i32NetNum = pcVideo->m_i32NetNum;
    FLOATU_T     f64ErrRate = pcVideo->m_f64Err;
    FLOATU_T     fTargetRate = pcVideo->m_fTargetRate;
    FLOATU_T     fResultRate = pcVideo->m_fResultRate;
    INT32_T      i32TargetNum = pcVideo->m_i32TargetNum;
    STRING_T     strNetFilePath[3] = {pcVideo->m_strNetFilePath1,pcVideo->m_strNetFilePath2,\
                                      pcVideo->m_strNetFilePath3};
    STRING_T     strNetConfigFile[3] = {pcVideo->m_strNetConfig1,pcVideo->m_strNetConfig2,\
                                        pcVideo->m_strNetConfig3};
    STRING_T     strGrayPngSuffix[4] = {"gray\\","right\\","right2\\","right3\\"};
    STRING_T     strPngFilePath = pcVideo->m_strPngFile;
    STRING_T     strResultPath  = pcVideo->m_strResultPath;
    UINT32_T     ui32VideoFlag  = pcVideo->m_ui32VideoFlag;
    INT32_T      i32ZipCols = pcVideo->m_i32cols;
    INT32_T      i32ZipRows = pcVideo->m_i32rows;
    StringList_V vFileName,vPngFileName;
    INT32_T      i32FileNum = 0,i32Ret = EXIT_OK;        

    NNTarget_V  **ppvTarget = new NNTarget_V*[i32NetNum];
    NNTarget_S  *psTarget = NULL;
    NNTrain_V   **ppvTrain = new NNTrain_V*[i32NetNum];
    NNTrain_S   *psTrain = NULL;
    CCNN        **ppcCNN = new CCNN*[i32NetNum];
    NNTest_S    *psTest = NULL;
    CNNScale_S   sScale[3];
    vector<UINT32_T> vInt;
    INT32_T     i32PngNum = 0;
    STRING_T    strLog;

    //加载网络文件
    CConfigText  *pcConfig = new CConfigText();
    for (INT32_T i32Index = INDEXBEGIN;i32Index < i32NetNum;i32Index++)
    {
        ppcCNN[i32Index] = new CCNN;
        ppvTrain[i32Index] = new NNTrain_V;
        ppvTarget[i32Index] = new NNTarget_V;

        InitNetFile(pcConfig,ppcCNN[i32Index],&sScale[i32Index],ppvTarget[i32Index],\
            ppvTrain[i32Index],strNetFilePath[i32Index],strNetConfigFile[i32Index]);
    }
    delete pcConfig;

    getJustCurrentFile(strFilePath,vFileName);
    i32FileNum = vFileName.size();

    while(BTRUE)
    {
        //从视频中获取png文件
        pcVideo->m_pcLog->LogToShell(LOG_INFO,"Start Video Analyze ....",BTRUE);
        for (INT32_T i32Index = INDEXBEGIN;i32Index < i32FileNum;i32Index++)
        {
            pcVideo->m_pcLog->LogToFile(LOG_INFO,"Start " + Int32ToString(i32Index+1)\
                +" Video Analyze, Name: " + vFileName[i32Index]);
            if (EXIT_OK != CreatePNG(strFilePath,vFileName[i32Index],strPngFilePath))
            {
                pcVideo->m_pcLog->LogToShell(LOG_ERROR,"CreatePNG Error",BTRUE);
                continue;
            }

            vPngFileName.clear();
            getJustCurrentFile(strPngFilePath,vPngFileName);

            if (EXIT_OK != ChangeRGB2GRAY(strPngFilePath,vPngFileName))
            {
                pcVideo->m_pcLog->LogToShell(LOG_ERROR,"ChangeRGB2GRAYNormal Error",BTRUE);
                continue;
            }

            vPngFileName.clear();
            getJustCurrentFile(strPngFilePath + strGrayPngSuffix[0],vPngFileName);

            if (EXIT_OK != ZipPng(strPngFilePath+strGrayPngSuffix[0],vPngFileName,i32ZipCols,i32ZipRows))
            {
                pcVideo->m_pcLog->LogToShell(LOG_ERROR,"ZipPng Error",BTRUE);
                continue;
            }

            //开始多层网络数据判断
            for (INT32_T i32NetIndex = INDEXBEGIN;i32NetIndex < i32NetNum;i32NetIndex++)
            {
                vPngFileName.clear();
                getJustCurrentFile(strPngFilePath + strGrayPngSuffix[i32NetIndex],vPngFileName);
                i32PngNum = vPngFileName.size();

                i32Ret = AnalyzeNetData(ppcCNN[i32NetIndex],ppvTarget[i32NetIndex],\
                    ppvTrain[i32NetIndex],ui32VideoFlag,strPngFilePath + strGrayPngSuffix[i32NetIndex],
                    strPngFilePath + strGrayPngSuffix[i32NetIndex+1],vPngFileName,pcVideo,&vInt,\
                    i32NetIndex+1,f64ErrRate,i32ZipCols,i32ZipRows,fTargetRate,i32TargetNum);
                if (i32Ret != EXIT_OK)
                {
                    break;
                }
            }

            strLog = "vInt " + Int32ToString(vInt.size()) + "  " \
            + FloatToString((FLOATU_T)vInt.size()/(i32PngNum));
            pcVideo->m_pcLog->LogToFile(LOG_INFO,strLog);
            if (i32PngNum != 0 && (FLOATU_T)vInt.size()/i32PngNum > fResultRate)  //已经获取到满足条件的结果值
            {
                STRING_T strRightFile = strFilePath + vFileName[i32Index];
                STRING_T strRightFile2 = strResultPath + vFileName[i32Index];

                CopyFile(strRightFile.c_str(),strRightFile2.c_str(),BFALSE);
                DeleteFile(strRightFile.c_str());
            }
        }
        pcVideo->m_pcLog->LogToShell(LOG_INFO,"Completed All Video Analyze ....",BTRUE);
        break;
    }

    for (INT32_T i32DeleteIndex = INDEXBEGIN;i32DeleteIndex<i32NetNum;i32DeleteIndex++)
    {
        delete ppvTrain[i32DeleteIndex];
        delete ppvTarget[i32DeleteIndex];
        delete []ppcCNN[i32DeleteIndex];
    }
    delete []ppvTrain;
    delete []ppvTarget;
    delete []ppcCNN;


    pcVideo->SetThreadEnd(((ThreadParameter_S*)psParam)->strThreadName);
    return EXIT_OK;
}

/** 从视频中生成网络文件所需的训练样本数据*/
UINT32_T ThreadSpace::VideoOpenCVThread(PVOID_T psParam)
{
    CVideoOpenCV *pcVideo = (CVideoOpenCV*)((ThreadParameter_S*)psParam)->pThreadPool;
    STRING_T     strFilePath = pcVideo->m_strFilePath;
    STRING_T     strPngFilePath = pcVideo->m_strPngFile;
    INT32_T      i32ZipCols = pcVideo->m_i32cols;
    INT32_T      i32ZipRows = pcVideo->m_i32rows;
    StringList_V vFileName,vPngFileName;
    INT32_T      i32FileNum = 0,i32Ret = EXIT_OK;

    //getJustCurrentFile(strFilePath,vFileName);
    //i32FileNum = vFileName.size();

    while(BTRUE)
    {
        //从视频中获取png文件
        pcVideo->m_pcLog->LogToShell(LOG_INFO,"Start Video Analyze ....",BTRUE);
        for (INT32_T i32Index = INDEXBEGIN;i32Index < 1/*i32FileNum*/;i32Index++)
        {
            pcVideo->m_pcLog->LogToFile(LOG_INFO,"Start " + Int32ToString(i32Index+1)\
                +" Video Analyze ....");
            //if (EXIT_OK != CreatePNG(strFilePath,vFileName[i32Index],strPngFilePath))
            //{
            //    pcVideo->m_pcLog->LogToShell(LOG_ERROR,"CreatePNG Error",BTRUE);
            //    continue;
            //}

            vPngFileName.clear();
            getJustCurrentFile(strPngFilePath,vPngFileName);

            if (EXIT_OK != ChangeRGB2GRAYNormal(strPngFilePath,vPngFileName))
            {
                pcVideo->m_pcLog->LogToShell(LOG_ERROR,"ChangeRGB2GRAYNormal Error",BTRUE);
                continue;
            }

            vPngFileName.clear();
            getJustCurrentFile(strPngFilePath,vPngFileName);

            if (EXIT_OK != ZipPngNormal(strPngFilePath,vPngFileName,i32ZipCols,i32ZipRows))
            {
                pcVideo->m_pcLog->LogToShell(LOG_ERROR,"ZipPng Error",BTRUE);
                continue;
            }

            pcVideo->m_pcLog->LogToFile(LOG_INFO,"Completed " + Int32ToString(i32Index+1)\
                + " Video Analyze ....");
        }
        pcVideo->m_pcLog->LogToShell(LOG_INFO,"Completed All Video Analyze ....",BTRUE);
        break;
    }
    
    pcVideo->SetThreadEnd(((ThreadParameter_S*)psParam)->strThreadName);
    return EXIT_OK;
}

/*
图片剪切线程
*/
UINT32_T ThreadSpace::PicutreCutThread(PVOID_T psParam)
{
    CVideoOpenCV *pcVideo = (CVideoOpenCV*)((ThreadParameter_S*)psParam)->pThreadPool;
    STRING_T     strPngFilePath = pcVideo->m_strPngFile;
    INT32_T      i32CutCols = pcVideo->m_i32cols;
    INT32_T      i32CutRows = pcVideo->m_i32rows;
    INT32_T      i32Move = pcVideo->m_i32Move;
    StringList_V vFileName,vPngFileName;

    while(BTRUE)
    {
        vPngFileName.clear();
        getJustCurrentFile(strPngFilePath,vPngFileName);

        if (EXIT_OK != CutPicture(strPngFilePath,vPngFileName,i32CutCols,i32CutRows,i32Move))
        {
            pcVideo->m_pcLog->LogToShell(LOG_ERROR,"CutPicture Error",BTRUE);
            continue;
        }
        break;
    }

    pcVideo->SetThreadEnd(((ThreadParameter_S*)psParam)->strThreadName);
    return EXIT_OK;
}
