/*****************************************************************************

        TestDownsampler2x4.h
        Author: Laurent de Soras, 2005

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (hiir_test_TestDownsampler2x4_HEADER_INCLUDED)
#define hiir_test_TestDownsampler2x4_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	<vector>



namespace hiir
{
namespace test
{



class SweepingSine;

template <class TO>
class TestDownsampler2x4
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	typedef TO TestedType;

	enum {         NBR_COEFS = TestedType::NBR_COEFS };

	static int     perform_test (TO &dspl, const double coef_arr [NBR_COEFS], const SweepingSine &ss, const char *type_0, double transition_bw, std::vector <float> &dest, double stopband_at);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               TestDownsampler2x4 ();
	               TestDownsampler2x4 (const TestDownsampler2x4 <TO> &other);
	TestDownsampler2x4 <TO> &
	               operator = (const TestDownsampler2x4 <TO> &other);
	bool           operator == (const TestDownsampler2x4 <TO> &other);
	bool           operator != (const TestDownsampler2x4 <TO> &other);

}; // class TestDownsampler2x4



}  // namespace test
}  // namespace hiir



#include "hiir/test/TestDownsampler2x4.hpp"



#endif   // hiir_test_TestDownsampler2x4_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
