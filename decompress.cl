__kernel void lzo1x_1_15_decompress(__global const unsigned char *in, __global unsigned char  *out,
					unsigned short NUM_THREADS,
					  unsigned int startidx, unsigned start_inp, unsigned short cpy_len,
					    unsigned short m_len, unsigned short m_off){
	
	int loops = cpy_len/NUM_THREADS;
	int rem = cpy_len%NUM_THREADS;
	int lcid = get_local_id(0);
	int i;
	for(i = 0; i < loops; i++){			//Parallelizing copying of unmatched literal data
		out[startidx + loops*lcid + i] = in[start_inp + loops*lcid + i];
	}
	startidx += loops*NUM_THREADS;
	start_inp += loops*NUM_THREADS;
	if(lcid < rem){
		out[startidx+lcid] = in[start_inp + lcid];
	}
	startidx += rem;				//serial copying of matched data until parallilization can be achieved
	int xtra = NUM_THREADS - m_off;
	if(xtra > 0){
		NUM_THREADS = m_off;
	}
	loops = m_len/NUM_THREADS;			//Parallelization achieved
	rem = m_len%NUM_THREADS;
	if(lcid < NUM_THREADS){
		for(i = 0; i < loops; i++){
			out[startidx + i*NUM_THREADS + lcid] = out[startidx + i*NUM_THREADS + lcid - m_off];
		}
		startidx += loops*NUM_THREADS;
		if(lcid < rem){
			out[startidx + lcid] = out[startidx + lcid - m_off];
		}
	}
}
