/*
ucl compression tool
Copyright (C) 2015, 2016 by Juan J. Martinez - usebox.net

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdio.h>
#include <string.h>

#include <ucl/ucl.h>

ucl_byte buffer[65536];

#define MAX_OVERLAP	512

int
main(int argc, char *argv[])
{
	ucl_uint in_len = 0;
	ucl_uint out_len, overlap, tmp_len;
	ucl_byte *in;
	ucl_byte *out;
	ucl_byte with_len = 0;

	if (argc != 1 && !strncmp(argv[1], "-s", 2))
		with_len = 1;

	if (ucl_init() != UCL_E_OK)
	{
		fprintf(stderr, "ucl: failed to init UCL\n");
		return 1;
	}

	while(!feof(stdin))
		buffer[in_len++] = getc(stdin);
	in_len--;

	out_len = in_len + in_len / 8 + 256;

	in = ucl_malloc(in_len + MAX_OVERLAP);
	out = ucl_malloc(out_len + MAX_OVERLAP);
	if (!in || !out)
	{
		fprintf(stderr, "ucl: out of memory\n");
		return 1;
	}

	memcpy(in, buffer, in_len);

	if (ucl_nrv2b_99_compress(in, in_len, out, &out_len, NULL, 10, NULL, NULL) != UCL_E_OK)
	{
		fprintf(stderr, "ucl: compress error\n");
		return 1;
	}

	if (out_len >= in_len)
	{
		fprintf(stderr, "ucl: content can't be compressed, output as-is (%d bytes)\n", in_len);
                if (with_len)
                        fwrite(&in_len, 2, 1, stdout);
                fwrite(buffer, 1, in_len, stdout);
                fclose(stdout);
                return 0;
	}

	if (with_len)
		fwrite(&out_len, 2, 1, stdout);

	fwrite(out, 1, out_len, stdout);
	fclose(stdout);

        /* test overlap */
        for (overlap = 0; overlap < MAX_OVERLAP; overlap++)
        {
                memcpy(in + overlap + in_len - out_len, out, out_len);
                tmp_len = in_len;
                if (ucl_nrv2b_test_overlap_8(in, overlap + in_len - out_len, out_len, &tmp_len, NULL) == UCL_E_OK && tmp_len == in_len)
                        break;
        }

	ucl_free(out);
	ucl_free(in);

	if (overlap == MAX_OVERLAP)
	{
		fprintf(stderr, "ucl: no valid overlap found\n");
		return 1;
	}

	fprintf(stderr, "ucl: %i bytes compressed into %i bytes (%i bytes slop)\n", in_len, out_len, overlap);

	return 0;
}


