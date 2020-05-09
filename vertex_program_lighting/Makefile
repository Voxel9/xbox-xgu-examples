XBE_TITLE = vertex_program_lighting
GEN_XISO = $(XBE_TITLE).iso

SRCS =  $(wildcard $(CURDIR)/*.c) \
        $(wildcard $(CURDIR)/../common/*.c)

SHADER_OBJS = combiner.inl vshader.inl

NXDK_DIR = $(CURDIR)/../../nxdk
NXDK_SDL = y

%.inl: %.ps $(FP20COMPILER)
	@echo "[ FP20     ] $@"
	$(FP20COMPILER) $< > $@

include $(NXDK_DIR)/Makefile
