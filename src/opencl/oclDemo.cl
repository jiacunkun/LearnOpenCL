kernel void GammaMap(global const uchar *src, global uchar *dst,
                     global const uchar *gammaLut, const int height, const int width)
{
    //获取当前图像行和列
    int row = get_global_id(1);
	int col = get_global_id(0) * 8;

    if (col >= 0 && col < width && row >= 0 && row < height)
    {
        uchar8 dstVal;
        global const uchar * pSrc = src + mad24(row, width, col);
        global uchar * pDst = dst + mad24(row, width, col);
        uchar8 srcVal = vload8(0, pSrc);
        dstVal.s0 = gammaLut[srcVal.s0];
        dstVal.s1 = gammaLut[srcVal.s1];
        dstVal.s2 = gammaLut[srcVal.s2];
        dstVal.s3 = gammaLut[srcVal.s3];
        dstVal.s4 = gammaLut[srcVal.s4];
        dstVal.s5 = gammaLut[srcVal.s5];
        dstVal.s6 = gammaLut[srcVal.s6];
        dstVal.s7 = gammaLut[srcVal.s7];
        vstore8(dstVal, 0, pDst);
    }
}

kernel void Histogram(global const uchar *src, global unsigned int *hist)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1);
}