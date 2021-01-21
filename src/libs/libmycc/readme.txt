* ****************************** *
* MyCC stack protocol (libmycc ) *
* ****************************** *

DER encoding and BER decoding !!!

ASN.1 free compiler:

$ asn1c -v

ASN.1 Compiler, v0.9.28
Copyright (c) 2003-2016 Lev Walkin <vlm@lionet.info>

Generated *.c and *.h source...libmycc compiling:

1.) asn1c ../MyCC.asn1
2.) copy 'MyCC.c' and 'MyCC.h' to ./src/
3.) gcc -DPDU=MyCC-PDU -I. -fPIC -c *.c
4.) gcc -shared -o libmycc.so *.o
5.) cp -vi libmycc.so /lib64/
6.) ldconfig

$ldconfig
$ldconfig -p | grep libmycc
	libmycc.so (libc6,x86-64) => /lib64/libmycc.so

7.) cp -v src/*.h include/

Test program:

$gcc -I./include/ -c main.c
$gcc -o myCC main.o -L. -lmycc


******
asn_enc_rval_t der_encode(struct asn_TYPE_descriptor_s *type_descriptor,
		void *struct_ptr,	/* Structure to be encoded */
		asn_app_consume_bytes_f *consume_bytes_cb,
		void *app_key		/* Arbitrary callback argument */
	);

der_encode(&asn_DEF_MyCC_PDU, myCC, write_out, fp);

asn_dec_rval_t ber_decode(struct asn_codec_ctx_s *opt_codec_ctx,
	struct asn_TYPE_descriptor_s *type_descriptor,
	void **struct_ptr,	/* Pointer to a target structure's pointer */
	const void *buffer,	/* Data to be decoded */
	size_t size		/* Size of that buffer */
	);
	
ber_decode(0, &asn_DEF_MyCC_PDU, (void **)&myCC_Dec, buf, 1024);

* записват и четат в и от  файл !
  как ще се използва за socket ???
  
asn_enc_rval_t
der_encode_to_buffer(asn_TYPE_descriptor_t *type_descriptor, void *struct_ptr,
	void *buffer, size_t buffer_size)

+ der_encode_to_buffer() вместо der_encode() !!!

* изпраща се целият buffer,който е 1024(MyCC-PDU + нули)

- как да се определи/вземе PDU_size а ?
  recognize_buffer_size() ,брой байтове докато стигне до 0(dec);   гаранция ли е това ?

********
my_request_balance(),hex string:

0000   a1 7d 30 7b a1 21 30 1f a1 03 02 01 03 a2 03 02
0010   01 01 a3 03 02 01 02 a4 06 02 04 59 88 5e 7c a5
0020   06 04 04 53 4d 53 31 a2 03 0a 01 01 a3 51 a1 4f
0030   30 4d a1 28 31 26 04 24 33 34 66 38 31 62 33 63
0040   2d 37 62 33 66 2d 31 31 65 37 2d 38 35 33 37 2d
0050   36 39 64 66 39 62 64 61 65 37 33 61 a2 10 31 0e
0060   04 0c 33 35 39 39 39 36 36 36 36 31 39 39 a3 0f
0070   31 0d 04 0b 33 35 39 32 34 31 31 39 39 39 38

********

