#-*-mode:makefile-gmake;indent-tabs-mode:t;tab-width:8;coding:utf-8-*-┐
#───vi: set et ft=make ts=8 tw=8 fenc=utf-8 :vi───────────────────────┘

PKGS += LIBC_ZIPOS

LIBC_ZIPOS_ARTIFACTS += LIBC_ZIPOS_A
LIBC_ZIPOS_A = o/$(MODE)/libc/zipos/zipos.a
LIBC_ZIPOS_A_FILES := $(wildcard libc/zipos/*)
LIBC_ZIPOS_A_HDRS = $(filter %.h,$(LIBC_ZIPOS_A_FILES))
LIBC_ZIPOS_A_SRCS_S = $(filter %.S,$(LIBC_ZIPOS_A_FILES))
LIBC_ZIPOS_A_SRCS_C = $(filter %.c,$(LIBC_ZIPOS_A_FILES))

LIBC_ZIPOS =						\
	$(LIBC_ZIPOS_A_DEPS)				\
	$(LIBC_ZIPOS_A)

LIBC_ZIPOS_A_SRCS =					\
	$(LIBC_ZIPOS_A_SRCS_S)				\
	$(LIBC_ZIPOS_A_SRCS_C)

LIBC_ZIPOS_A_OBJS =					\
	$(LIBC_ZIPOS_A_SRCS_S:%.S=o/$(MODE)/%.o)	\
	$(LIBC_ZIPOS_A_SRCS_C:%.c=o/$(MODE)/%.o)	\
	o/$(MODE)/libc/zipos/.cosmo.zip.o

LIBC_ZIPOS_A_CHECKS =					\
	$(LIBC_ZIPOS_A).pkg				\
	$(LIBC_ZIPOS_A_HDRS:%=o/$(MODE)/%.ok)

LIBC_ZIPOS_A_DIRECTDEPS =				\
	LIBC_CALLS					\
	LIBC_FMT					\
	LIBC_INTRIN					\
	LIBC_MEM					\
	LIBC_NEXGEN32E					\
	LIBC_NT_KERNEL32				\
	LIBC_RUNTIME					\
	LIBC_STR					\
	LIBC_SYSV					\
	LIBC_SYSV_CALLS					\
	LIBC_THREAD					\
	THIRD_PARTY_PUFF

LIBC_ZIPOS_A_DEPS :=					\
	$(call uniq,$(foreach zipos,$(LIBC_ZIPOS_A_DIRECTDEPS),$($(zipos))))

$(LIBC_ZIPOS_A):libc/zipos/				\
		$(LIBC_ZIPOS_A).pkg			\
		$(LIBC_ZIPOS_A_OBJS)

$(LIBC_ZIPOS_A).pkg:					\
		$(LIBC_ZIPOS_A_OBJS)			\
		$(foreach zipos,$(LIBC_ZIPOS_A_DIRECTDEPS),$($(zipos)_A).pkg)

o/$(MODE)/libc/zipos/.cosmo.zip.o: private		\
		ZIPOBJ_FLAGS +=				\
			-B

# these assembly files are safe to build on aarch64
o/$(MODE)/libc/zipos/zipos.o: libc/zipos/zipos.S
	@$(COMPILE) -AOBJECTIFY.S $(OBJECTIFY.S) $(OUTPUT_OPTION) -c $<

LIBC_ZIPOS_LIBS = $(foreach zipos,$(LIBC_ZIPOS_ARTIFACTS),$($(zipos)))
LIBC_ZIPOS_SRCS = $(foreach zipos,$(LIBC_ZIPOS_ARTIFACTS),$($(zipos)_SRCS))
LIBC_ZIPOS_HDRS = $(foreach zipos,$(LIBC_ZIPOS_ARTIFACTS),$($(zipos)_HDRS))
LIBC_ZIPOS_BINS = $(foreach zipos,$(LIBC_ZIPOS_ARTIFACTS),$($(zipos)_BINS))
LIBC_ZIPOS_CHECKS = $(foreach zipos,$(LIBC_ZIPOS_ARTIFACTS),$($(zipos)_CHECKS))
LIBC_ZIPOS_OBJS = $(foreach zipos,$(LIBC_ZIPOS_ARTIFACTS),$($(zipos)_OBJS))
LIBC_ZIPOS_TESTS = $(foreach zipos,$(LIBC_ZIPOS_ARTIFACTS),$($(zipos)_TESTS))
$(LIBC_ZIPOS_OBJS): $(BUILD_FILES) libc/zipos/zipos.mk

.PHONY: o/$(MODE)/libc/zipos
o/$(MODE)/libc/zipos: $(LIBC_ZIPOS_CHECKS)
