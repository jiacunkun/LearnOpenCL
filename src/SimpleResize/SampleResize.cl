__kernel void resize_8uc1/*global_size={dst_cols, dst_rows}*/(
	const __global unsigned char *src,
	int src_step,
	int src_cols,
	int src_rows,
	__global unsigned char *dst,
	int dst_step,
	int dst_cols,
	int dst_rows	
)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

    const float ratiox = ((float)src_cols) / ((float)dst_cols);
    const float ratioy =  ((float)src_rows) / ((float)dst_rows);

	///
	float src_y = ((float)y + 0.5f) * ratioy - 0.5f;
	int src_y_floor = (int)floor(src_y);
    src_y_floor = clamp(src_y_floor, 0, src_rows-2);
	float beta = src_y - (float)src_y_floor;
    beta = clamp(beta,0.0f, 1.0f);
    int src_y_floor_1 = src_y_floor + 1;


	/////
	float src_x = ((float)x + 0.5f) * ratiox - 0.5f;
	int src_x_floor = (int)floor(src_x);
	src_x_floor = clamp(src_x_floor,0, src_cols-2);
	float alpha = src_x - (float)src_x_floor;
    alpha = clamp(alpha, 0.0f, 1.0f);
    int src_x_floor_1 = src_x_floor + 1;

	// Adreno(TM) 612:  38 ms - 42ms
    /*float A00 = (float)src[src_y_floor*src_step + src_x_floor];
	float A01 = (float)src[src_y_floor*src_step + src_x_floor_1];
	float A10 = (float)src[src_y_floor_1*src_step + src_x_floor];
	float A11 = (float)src[src_y_floor_1*src_step + src_x_floor_1];	
	

	float A00f = A00 + (A01 - A00) * alpha;
	float A10f = A10 + (A11 - A10) * alpha;
	dst[y*dst_step + x] = (unsigned char)(A00f + (A10f - A00f) * beta);*/
	
	// Adreno(TM) 612:  25 ms - 30ms
	float2 A00 = convert_float2(vload2(0, src + mad24(src_y_floor, src_step, src_x_floor)));
    float2 A10 = convert_float2(vload2(0, src + mad24(src_y_floor_1, src_step , src_x_floor)));
	float2 v_alpha = (float2)(1.0f - alpha, alpha );
	float a0 = dot(A00, v_alpha);
	float a1 = dot(A10, v_alpha);	
	dst[y*dst_step + x] = (unsigned char)(a0 + (a1 - a0) * beta);
}