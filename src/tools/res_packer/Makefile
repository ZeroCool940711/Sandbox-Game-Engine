
BIN  = res_packer
SRCS =					\
	cdata_packer		\
	chunk_packer		\
	font_packer			\
	fx_packer			\
	image_packer		\
	main				\
	model_anim_packer	\
	msg_handler			\
	packer_helper		\
	packers				\
	xml_packer			\

INSTALL_DIR = $(MF_ROOT)/bigworld/tools/misc/res_packer

ASMS =

MY_LIBS =

USE_PYTHON = 1

ifndef MF_ROOT
export MF_ROOT := $(subst /src/tools/res_packer,,$(CURDIR))
endif

include $(MF_ROOT)/bigworld/src/server/common/common.mak

