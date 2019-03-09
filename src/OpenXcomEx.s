	.global common_zip
	.global common_zip_size
	.global standard_zip
	.global standard_zip_size
	.section .rodata
common_zip:
	.incbin "bin/common.zip"
1:
standard_zip:
	.incbin "bin/standard.zip"
2:
common_zip_size:
	.int 1b - common_zip
standard_zip_size:
	.int 2b - standard_zip
