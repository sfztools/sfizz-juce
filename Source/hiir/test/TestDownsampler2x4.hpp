/*****************************************************************************

        TestDownsampler2x4.hpp
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (hiir_test_TestDownsampler2x4_CURRENT_CODEHEADER)
	#error Recursive inclusion of TestDownsampler2x4 code header.
#endif
#define hiir_test_TestDownsampler2x4_CURRENT_CODEHEADER

#if ! defined (hiir_test_TestDownsampler2x4_CODEHEADER_INCLUDED)
#define hiir_test_TestDownsampler2x4_CODEHEADER_INCLUDED



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "hiir/test/BlockSplitter.h"
#include "hiir/test/FileOp.h"
#include "hiir/test/ResultCheck.h"
#include "hiir/test/SweepingSine.h"

#include <cassert>
#include <cstdio>



namespace hiir
{
namespace test
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



template <class TO>
int	TestDownsampler2x4 <TO>::perform_test (TO &dspl, const double coef_arr [NBR_COEFS], const SweepingSine &ss, const char *type_0, double transition_bw, std::vector <float> &dest, double stopband_at)
{
	assert (coef_arr != 0);
	assert (type_0 != 0);
	assert (transition_bw > 0);
	assert (transition_bw < 0.5);
	assert (stopband_at > 0);

	printf (
		"Test: Downsampler2x4, %s implementation, %d coefficients.\n",
		type_0,
		NBR_COEFS
	);

	const long     len = ss.get_len ();
	std::vector <float>	src_base (len);
	printf ("Generating sweeping sine... ");
	fflush (stdout);
	ss.generate (&src_base [0]);
	std::vector <float>  src (len * 4);
	for (long pos = 0; pos <len; ++pos)
	{
		for (int chn = 0; chn < 4; ++chn)
		{
			src [pos * 4 + chn] = src_base [pos];
		}
	}
	printf ("Done.\n");

	dspl.set_coefs (coef_arr);
	dspl.clear_buffers ();

	const long     len_proc = len / 2;
	dest.resize (len_proc * 4, 0);

	printf ("Downsampling... ");
	fflush (stdout);
	BlockSplitter	bs (64);
	for (bs.start (len_proc); bs.is_continuing (); bs.set_next_block ())
	{
		const long     b_pos = bs.get_pos ();
		const long     b_len = bs.get_len ();
		dspl.process_block (&dest [b_pos * 4], &src [b_pos * 8], b_len);
	}
	printf ("Done.\n");

	int            ret_val = 0;
	char           filename_0 [255+1];
	std::vector <float>  dst_chk (len_proc);
	for (int chn = 0; chn < 4 && ret_val == 0; ++chn)
	{
		for (long pos = 0; pos < len_proc; ++pos)
		{
			dst_chk [pos] = dest [pos * 4 + chn];
		}
		ret_val = ResultCheck::check_dspl (
			ss,
			transition_bw,
			stopband_at,
			&dst_chk [0]
		);

		sprintf (
			filename_0, "dspl_%02d_%s_4x-%01d.raw",
			TestedType::NBR_COEFS, type_0, chn
		);
		FileOp::save_raw_data_16 (filename_0, &dst_chk [0], len_proc, 1);
	}

	printf ("\n");

	return ret_val;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace test
}  // namespace hiir



#endif   // hiir_test_TestDownsampler2x4_CODEHEADER_INCLUDED

#undef hiir_test_TestDownsampler2x4_CURRENT_CODEHEADER



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
