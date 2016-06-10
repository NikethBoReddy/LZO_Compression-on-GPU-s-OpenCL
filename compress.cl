/* Author: Niketh Boreddy
   email: nikethcse@gmail.com
   
  Date: 10th June 2016
  The follwing file is a part of the LZO algortihm implementation for GPU's
*/

#define HashTbl_entries 8192
#define lookahead 128
#define OPT5 1		 //Change to zero for disabling Opt#5

__kernel void lzo1x_1_15_compress(__global const unsigned char *in, unsigned int in_size,
			__global unsigned char  *out, unsigned int out_size,
			  unsigned int block_size, int blocksPerLaunch,
			    __global unsigned int *end_pts,
			     __global unsigned int *Debug,
			       __global unsigned short *HashTable,
			       int iter){
	
	unsigned short wrkgrp = get_group_id(0);
	unsigned int gbl_wrkgrp = wrkgrp + iter*blocksPerLaunch;
	unsigned int startidx_inp = gbl_wrkgrp*block_size;
	unsigned short blks = in_size%block_size == 0 ? in_size/block_size : in_size/block_size + 1;
	unsigned int startidx_out = gbl_wrkgrp*(out_size/blks);
	
	short NUM_THREADS = get_local_size(0);
	short pl = lookahead/NUM_THREADS;

	unsigned short lcid = get_local_id(0);
	unsigned short data_sz = block_size > in_size - startidx_inp ? in_size - startidx_inp : block_size;
	unsigned int Hashidx = gbl_wrkgrp*HashTbl_entries;
	HashTable += Hashidx;
	unsigned short loops = HashTbl_entries/NUM_THREADS;
	unsigned short rem = HashTbl_entries%NUM_THREADS;
	unsigned short i;
	for(i = 0; i < loops; i++){
		HashTable[lcid + i*NUM_THREADS] = 0;
	}
	if(lcid < rem){
		HashTable[loops*NUM_THREADS + lcid] = 0;
	}
	unsigned short D_bits = 0;
	unsigned short Hashtable_entries = HashTbl_entries; 
	while(Hashtable_entries != 0){
		D_bits++;
		Hashtable_entries /= 2;
	}
	D_bits--;
	unsigned int endidx_inp = startidx_inp + data_sz;
	unsigned int stopidx_inp = startidx_inp + data_sz - 20;
	unsigned int lastMatch_inp = startidx_inp;
	unsigned int curridx_inp = startidx_inp;
	unsigned int curridx_out = startidx_out;
	unsigned int m_pos;
	unsigned short m_len, m_off;
	unsigned int dv;
	unsigned short dindex;
	curridx_inp += 4;
	
	unsigned short t = 0;
	unsigned short flag = 1;
	__local char test[8];
	if(lcid < 8){
		test[lcid] = 0;
	}
	__local unsigned char vals[lookahead+3];
	__local unsigned int dvs[lookahead];
	unsigned int cacheidx;
	loops = pl;
	if(data_sz >= lookahead+3){
		cacheidx = startidx_inp + 5;
		for(i = 0; i < loops; i++){
			vals[NUM_THREADS*i + lcid] = in[startidx_inp + NUM_THREADS*i + lcid + 5];
		}
		if(lcid < 3){
			vals[NUM_THREADS*loops + lcid] = in[startidx_inp + NUM_THREADS*i + lcid + 5];
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		for(i = 0; i < loops; i++){
			unsigned int cacheval = loops*lcid + i;
            		dvs[cacheval] = ((unsigned int)vals[cacheval] | ((unsigned int)vals[cacheval+1] << 8) | ((unsigned int)vals[cacheval+2] << 16) | ((unsigned int)vals[cacheval+3] << 24));
		}
		#if OPT5
		for(i = 0; i < loops; i++){
			unsigned int cacheval = ((loops-i)*NUM_THREADS -1) - lcid;
			if(lcid > NUM_THREADS/2)HashTable[((1 << D_bits)-1)&(((unsigned int)(0x1824429d*dvs[cacheval])) >> (32-D_bits))] = (unsigned short)(cacheidx + cacheval - startidx_inp);
		}
		#endif
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	char lookaheadflag = 0;
	char stopcache = 0;
	for(;;){
		if(flag)curridx_inp += 1 + ((curridx_inp - lastMatch_inp) >> 6);
		else flag = 1;
		if(curridx_inp >= stopidx_inp) break;
		if(!stopcache && lookaheadflag){		//Optimization #4
			unsigned short len = stopidx_inp - curridx_inp > lookahead+3 ? lookahead+3 : 0;
			if(len){
				cacheidx = curridx_inp;
				loops = pl;
				for(i = 0; i < loops; i++) vals[NUM_THREADS*i + lcid] = in[curridx_inp + NUM_THREADS*i + lcid];
				if(lcid < 3)vals[loops*NUM_THREADS + lcid] = in[curridx_inp + loops*NUM_THREADS + lcid];
				barrier(CLK_LOCAL_MEM_FENCE);
				for(i = 0; i < loops; i++){
					unsigned int cacheval = loops*lcid + i;
			    		dvs[cacheval] = ((unsigned int)vals[cacheval] | ((unsigned int)vals[cacheval+1] << 8) | ((unsigned int)vals[cacheval+2] << 16) | ((unsigned int)vals[cacheval+3] << 24));
				}
				#if OPT5		//Optimization #5
				for(i = 0; i < loops; i++){
					unsigned int cacheval = ((loops-i)*NUM_THREADS - 1) - lcid;
					if(lcid > NUM_THREADS/2)HashTable[((1 << D_bits)-1)&(((unsigned int)(0x1824429d*dvs[cacheval])) >> (32-D_bits))] = (unsigned short)(cacheidx + cacheval - startidx_inp);
				}
				#endif
				lookaheadflag = 0;
			}
			else stopcache = 1;
			barrier(CLK_LOCAL_MEM_FENCE);
		}
		short offset = curridx_inp - cacheidx;
		if(!stopcache && offset < lookahead){
			dv = dvs[offset];			//Optimization #4
		}
		else{
			dv = ((unsigned int)in[curridx_inp] | ((unsigned int)in[curridx_inp+1] << 8) | ((unsigned int)in[curridx_inp+2] << 16) | ((unsigned int)in[curridx_inp+3] << 24));
			lookaheadflag = 1;
		}
		dindex = ((1 << D_bits)-1)&(((unsigned int)(0x1824429d*dv)) >> (32-D_bits));
		m_pos = startidx_inp + HashTable[dindex];
		barrier(CLK_LOCAL_MEM_FENCE);
		if(m_pos == curridx_inp) continue;
		if(lcid == 0)HashTable[dindex] = (unsigned short)(curridx_inp - startidx_inp);
		unsigned int match_val;
		match_val = ((unsigned int)in[m_pos] | ((unsigned int)in[m_pos+1] << 8) | ((unsigned int)in[m_pos+2] << 16) | ((unsigned int)in[m_pos+3] << 24));
		if(dv != match_val)continue;
			
		if(m_pos > curridx_inp){
			unsigned int temp = m_pos;
			m_pos = curridx_inp;
			curridx_inp = temp;
		}
		t = (unsigned short)(curridx_inp - lastMatch_inp);
		if(t){
			if(t <= 3){
				if(lcid == 0) out[curridx_out-2] = (unsigned char)(out[curridx_out-2] | t);
				do{
					if(lcid == 0) out[curridx_out] = in[lastMatch_inp];
					curridx_out++;
					lastMatch_inp++;
				}
				while(--t > 0);
			}
			else{ 
				if(t <= 18){
					if(lcid == 0) out[curridx_out] = (unsigned char)(t-3);
					curridx_out++;
				}
				else{
					unsigned short tt = t-18;
					if(lcid == 0)out[curridx_out] = 0;
					curridx_out++;
					while(tt > 255){
						tt -= 255;
						if(lcid == 0)out[curridx_out] = 0;
						curridx_out++;
					}
					if(lcid == 0)out[curridx_out] = (unsigned char)tt;
					curridx_out++;
				}
				unsigned short loops = t/NUM_THREADS;
				unsigned short rem = t%NUM_THREADS;
				unsigned short i;
				for(i = 0; i < loops; i++){
					out[curridx_out + NUM_THREADS*i + lcid] = in[lastMatch_inp + NUM_THREADS*i + lcid];
				}
				curridx_out += loops*NUM_THREADS;
				lastMatch_inp += loops*NUM_THREADS;
				if(lcid < rem){
					out[curridx_out + lcid] = in[lastMatch_inp + lcid];
				}
				curridx_out += rem;
				lastMatch_inp += rem;
			}
		}
		m_len = 4;
		while(1){
			if(lcid < 8){
				test[lcid] = (in[m_pos + m_len + lcid] == in[curridx_inp + m_len + lcid]);
			}
			barrier(CLK_LOCAL_MEM_FENCE);
			char lenidx;
			for(lenidx = 0; lenidx < 8; lenidx++){
				if(!test[lenidx])break;
				m_len += 1;
			}
			if(lenidx < 8)break;
			if(curridx_inp + m_len >= stopidx_inp) break;
		}
		
		m_off = curridx_inp - m_pos;
		curridx_inp += m_len;
		lastMatch_inp = curridx_inp;
		if(m_len <= 8 && m_off <= 0x800){
			m_off -= 1;
			if(lcid == 0){
				out[curridx_out] = (unsigned char)(((m_len-1) << 5) | ((m_off & 7) << 2));
				out[curridx_out+1] = (unsigned char)(m_off >> 3);
			}
			curridx_out += 2;
		}
		else if(m_off <= 0x4000){
			m_off -= 1;
			if(m_len <= 33){
				if(lcid == 0) out[curridx_out] = (unsigned char)(32 | (m_len-2));
				curridx_out++;
			}
			else{
				m_len -= 33;
				if(lcid == 0)out[curridx_out] = (unsigned char)(32|0);
				curridx_out++;
				while(m_len > 255){
					m_len -= 255;
					if(lcid == 0)out[curridx_out] = 0;
					curridx_out++;
				}
				if(lcid == 0) out[curridx_out] = (unsigned char)(m_len);
				curridx_out++;
			}
			if(lcid == 0){
				out[curridx_out] = (unsigned char)(m_off << 2);
				out[curridx_out+1] = (unsigned char)(m_off >> 6);
			}
			curridx_out += 2;
		}
		else{
			m_off -= 0x4000;
			if(m_len <= 9){
				if(lcid == 0)out[curridx_out] = (unsigned char)(16 | ((m_off >> 11)&8) | (m_len-2));
				curridx_out++;
			}
			else{
				m_len -= 9;
				if(lcid == 0)out[curridx_out] = (unsigned char) (16 | ((m_off >>11)&8));
				curridx_out++;
				while(m_len > 255){
					m_len -= 255;
					if(lcid == 0)out[curridx_out] = 0;
					curridx_out++;
				}
				if(lcid == 0)out[curridx_out] = (unsigned char)(m_len);
				curridx_out++;
			}
			if(lcid == 0){
				out[curridx_out] = (unsigned char)(m_off << 2);
				out[curridx_out+1] = (unsigned char)(m_off >> 6);
			}
			curridx_out += 2;
		}
		flag = 0;
	}
	
	t = (unsigned short)(endidx_inp - lastMatch_inp);
	if(t > 0){
		if(t <= 3){
			if(lcid == 0)out[curridx_out-2] = (unsigned char)(out[curridx_out-2] | t);
		}
		else if(t <= 18){
			if(lcid == 0)out[curridx_out] = (unsigned char)(t-3);
			curridx_out++;
		}
		else{
			unsigned short tt = t-18;
			if(lcid == 0)out[curridx_out] = 0;
			curridx_out++;
			while(tt > 255){
				tt -= 255;
				if(lcid == 0)out[curridx_out] = 0;
				curridx_out++;
			}
			if(lcid == 0)out[curridx_out] = (unsigned char) tt;
			curridx_out++;
		}
		unsigned short loops = t/NUM_THREADS;
		unsigned short rem = t%NUM_THREADS;
		unsigned short i;
		for(i = 0; i < loops; i++){
			out[curridx_out + lcid + i*NUM_THREADS] = in[lastMatch_inp + lcid + i*NUM_THREADS];
		}
		curridx_out += loops*NUM_THREADS;
		lastMatch_inp += loops*NUM_THREADS;
		if(lcid < rem){
			out[curridx_out + lcid] = in[lastMatch_inp + lcid];
		}
		curridx_out += rem;
		lastMatch_inp += rem;
	}
	
	if(lcid == 0){					//End Token
		out[curridx_out++] = 16 | 1;
		out[curridx_out++] = 0;
		out[curridx_out++] = 0;
		end_pts[gbl_wrkgrp] = (curridx_out - startidx_out);
	}
}
