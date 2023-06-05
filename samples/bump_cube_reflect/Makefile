XGU_EXAMPLES_DIR = ../..
include $(XGU_EXAMPLES_DIR)/common/xgu_sample_rules

XBE_TITLE = bump_cube_reflect
GEN_XISO = $(XBE_TITLE).iso

SRCS += $(wildcard $(CURDIR)/source/*.cpp)

SHADER_OBJS += source/combiner.inl \
               source/vshader.inl

include $(NXDK_DIR)/Makefile
