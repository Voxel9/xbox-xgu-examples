XBE_TITLE = alpha_blending
GEN_XISO = $(XBE_TITLE).iso

SRCS =  $(wildcard $(CURDIR)/*.c) \
        $(wildcard $(CURDIR)/../common/*.c)

SHADER_OBJS = combiner.inl vshader.inl

NXDK_DIR = $(CURDIR)/../../nxdk
NXDK_SDL = y

%.inl: %.ps | fp20compiler
	@echo "[ FP20     ] $@"
	$(FP20COMPILER) $< > $@

include $(NXDK_DIR)/Makefile
