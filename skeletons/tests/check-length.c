#include <ber_decoder.c>
#include <ber_tlv_length.c>
#include <ber_tlv_tag.c>
#include <der_encoder.c>
#include <constraints.c>
#undef	ADVANCE
#undef	RETURN
#undef	LEFT
#include <OCTET_STRING.c>


uint8_t *buf;
size_t buf_size;
size_t buf_off;

static int
write_to_buf(const void *buffer, size_t size, void *key) {
	(void)key;

	if(buf_off + size > buf_size) {
		size_t n = buf_size?:16;
		while(n < buf_off + size) n <<= 2;
		buf = realloc(buf, n);
		assert(buf);
		buf_size = n;
	}

	memcpy(buf + buf_off, buffer, size);

	buf_off += size;
	return 0;
}


static void
check(int size) {
	OCTET_STRING_t *os;
	OCTET_STRING_t *nos = 0;
	asn_enc_rval_t erval;
	ber_dec_rval_t rval;
	int i;

	os = OCTET_STRING_new_fromBuf(0, size);
	assert(os);
	assert(os->size == 0);

	os->buf = malloc(size);
	assert(os->buf);
	os->size = size;

	for(i = 0; i < size; i++) {
		os->buf[i] = i;
	}

	buf_off = 0;
	erval = der_encode(&asn_DEF_OCTET_STRING,
		os, write_to_buf, 0);
	assert(erval.encoded == buf_off);
	assert(buf_off > size);

	rval = ber_decode(0, &asn_DEF_OCTET_STRING, (void **)&nos, buf, buf_off);
	assert(rval.code == RC_OK);
	assert(rval.consumed == buf_off);

	assert(os->size == nos->size);

	for(i = 0; i < size; i++) {
		assert(os->buf[i] == nos->buf[i]);
	}

	if(0) {
	printf("new(%d):", size);
	for(i = 0; i < (buf_off<10?buf_off:10); i++)
		printf(" %02x", buf[i]);
	printf("\n");
	}


	asn_DEF_OCTET_STRING.free_struct(&asn_DEF_OCTET_STRING, os, 0);
	asn_DEF_OCTET_STRING.free_struct(&asn_DEF_OCTET_STRING, nos, 0);
}

int
main() {
	uint8_t buf1[] = { 0x85, 0x00, 0x01, 0x02, 0x03, 0x04 };
	uint8_t buf2[] = { 0x85, 0x00, 0x7f, 0xff, 0x03, 0x04 };
	uint8_t buf3[] = { 0x85, 0x00, 0x7f, 0xff, 0xff, 0x04 };
	ber_tlv_len_t tlv_len;
	ssize_t ret;
	int i;

	for(i = 0; i < 66000; i++) {
		if(i == 4500) i = 64000;
		check(i);
	}

	ret = ber_fetch_length(0, buf1, sizeof(buf1), &tlv_len);
	printf("ret=%d, len=%d\n", ret, tlv_len);
	assert(ret == sizeof(buf1));

	ret = ber_fetch_length(0, buf2, sizeof(buf2), &tlv_len);
	printf("ret=%d, len=%d\n", ret, tlv_len);
	assert(ret == sizeof(buf2));

	ret = ber_fetch_length(0, buf3, sizeof(buf3), &tlv_len);
	printf("ret=%d\n", ret);
	assert(ret == -1);

	return 0;
}
