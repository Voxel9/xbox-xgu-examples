XBE_TITLE = bumpenv_ripple
GEN_XISO = $(XBE_TITLE).iso

SRCS =  $(wildcard $(CURDIR)/src/*.c) \
        $(wildcard $(CURDIR)/../common/*.c)

SHADER_OBJS =   src/shaders/vshader.inl \
                src/shaders/ripple_frag.inl \
                src/shaders/diffuse_frag.inl

NXDK_DIR = $(CURDIR)/../../nxdk
NXDK_SDL = y

%.inl: %.ps | fp20compiler
	@echo "[ FP20     ] $@"
	$(FP20COMPILER) $< > $@

include $(NXDK_DIR)/Makefile
