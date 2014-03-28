// DEMCheck.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <gdal_priv.h>

#define byte unsigned char

char* inputFilePath = "F:\\数据\\srtm_59_07.tif";
double minx, miny, maxx, maxy, dx, dy;
int xSize, ySize;

const static int ERR_DEM_VALUE = -10000;

int GetSRTMDEMValue(double x, double y)
{
	char* basePath = "F:\\数据\\影像&DEM数据\\全球DEM\\全球DEM\\DEM\\SRTM\\";
	char destPath[_MAX_PATH];
	// 计算经纬度点对应的SRTM文件的行列编号
	for (int i = 1; i <= 24; i++)
	{
		for (int j = 1; j <= 72; j++)
		{
			if (y > 60 - (i -1) * 5 || y <= 60 - i * 5 || x < -180 + (j - 1) * 5 || x >= -180 + j * 5)
			{
				continue;
			}
			sprintf_s(destPath, "%s%02d\\srtm_%02d_%02d.tif", basePath, i, j, i);

			//尝试打开文件，如果无法打开，则直接返回错误信息
			GDALDataset* poDataset = (GDALDataset*)GDALOpen(destPath, GA_ReadOnly);
			if (NULL == poDataset)
			{
				return 0;
			}

			GDALDataType dataType = poDataset->GetRasterBand(1)->GetRasterDataType();
			// 获取图像信息
			double geoTransDem[6];
			poDataset->GetGeoTransform(geoTransDem);
			double nodata = poDataset->GetRasterBand(1)->GetNoDataValue();
			
			//计算经纬度对应的图像像素偏移量
			int xOffset = (int)((x - geoTransDem[0]) / geoTransDem[1] + 0.5);
			int yOffset = (int)((y - geoTransDem[3]) / geoTransDem[5] + 0.5);

			//读取图像中的对应值
			short pBuf = 0;
			CPLErr result = poDataset->RasterIO(GF_Read,
				xOffset,
				xOffset,
				1,
				1,
				&pBuf,
				1,
				1,
				GDT_Int16,
				1,
				NULL,
				0,
				0,
				0);

			if (abs(pBuf - (-32768)) < 0.1)
			{
				return 0;
			}

			if (CE_Failure == result)
			{
				return ERR_DEM_VALUE;
			}

			GDALClose((GDALDatasetH)poDataset);
			return pBuf;
		}
	}

	return ERR_DEM_VALUE;
}

int _tmain(int argc, _TCHAR* argv[])
{
	GDALAllRegister();
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
	GDALDataset* poDataset = (GDALDataset*)GDALOpen(inputFilePath, GA_ReadOnly);
	if (NULL == poDataset)
	{
		printf("无法打开文件");
		return 0;
	}

	xSize = poDataset->GetRasterXSize();
	ySize = poDataset->GetRasterYSize();

	GDALDataType dataType = poDataset->GetRasterBand(1)->GetRasterDataType();
	double geoTrans[6];
	poDataset->GetGeoTransform(geoTrans);
	minx = geoTrans[0];
	maxx = geoTrans[0] + xSize * geoTrans[1];
	maxy = geoTrans[3];
	miny = geoTrans[3] + ySize * geoTrans[5];
	dx = geoTrans[1];
	dy = abs(geoTrans[5]);
	int bandCount = poDataset->GetRasterCount();

	short* pBuf = new short[xSize * ySize];
	poDataset->RasterIO(GF_Read,
		0,
		0,
		xSize,
		ySize,
		pBuf,
		xSize,
		ySize,
		GDT_Int16,
		1,
		NULL,
		0,
		0,
		0);

	FILE* fp;
	fopen_s(&fp, "out.txt", "w");
	for (int x = 0; x < xSize; x++)
	{
		for (int y = 0; y < ySize; y++)
		{
			double geox = geoTrans[0] + x * geoTrans[1];
			double geoy = geoTrans[3] + y * geoTrans[5];
			short srtmDem = GetSRTMDEMValue(geox + dx / 2, geoy + dy / 2);
			if (ERR_DEM_VALUE == srtmDem)
			{
				printf("获取DEM高程值出现错误\n");
				continue;
			}
			int delta = pBuf[x + y * xSize] - GetSRTMDEMValue(geox, geoy);
			fprintf_s(fp, "%d\n", delta);
		}
	}

	fclose(fp);

	delete[] pBuf;
	pBuf = NULL;
	GDALClose((GDALDatasetH)poDataset);

	system("Pause");
	return 0;
}

