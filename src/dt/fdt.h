/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright 2013-2023 The DT-Utils Authors <oss-tools@pengutronix.de> */

#ifndef _FDT_H
#define _FDT_H

#ifndef __ASSEMBLY__

#define _B(n)	((unsigned long long)((uint8_t *)&x)[n])

#define fdt32_to_cpu(x)	be32_to_cpu(x)
#define cpu_to_fdt32(x) cpu_to_be32(x)

static inline uint64_t fdt64_to_cpu(uint64_t x)
{
	return (_B(0) << 56) | (_B(1) << 48) | (_B(2) << 40) | (_B(3) << 32)
		| (_B(4) << 24) | (_B(5) << 16) | (_B(6) << 8) | _B(7);
}
#define cpu_to_fdt64(x) fdt64_to_cpu(x)
#undef _B

struct fdt_header {
	uint32_t magic;			 /* magic word FDT_MAGIC */
	uint32_t totalsize;		 /* total size of DT block */
	uint32_t off_dt_struct;		 /* offset to structure */
	uint32_t off_dt_strings;	 /* offset to strings */
	uint32_t off_mem_rsvmap;	 /* offset to memory reserve map */
	uint32_t version;		 /* format version */
	uint32_t last_comp_version;	 /* last compatible version */

	/* version 2 fields below */
	uint32_t boot_cpuid_phys;	 /* Which physical CPU id we're
					    booting on */
	/* version 3 fields below */
	uint32_t size_dt_strings;	 /* size of the strings block */

	/* version 17 fields below */
	uint32_t size_dt_struct;	 /* size of the structure block */
};

struct fdt_reserve_entry {
	uint64_t address;
	uint64_t size;
};

struct fdt_node_header {
	uint32_t tag;
	char name[0];
};

struct fdt_property {
	uint32_t tag;
	uint32_t len;
	uint32_t nameoff;
	char data[0];
};

#endif /* !__ASSEMBLY */

#define FDT_MAGIC	0xd00dfeed	/* 4: version, 4: total size */
#define FDT_TAGSIZE	sizeof(uint32_t)

#define FDT_BEGIN_NODE	0x1		/* Start node: full name */
#define FDT_END_NODE	0x2		/* End node */
#define FDT_PROP	0x3		/* Property: name off,
					   size, content */
#define FDT_NOP		0x4		/* nop */
#define FDT_END		0x9

#define FDT_V1_SIZE	(7*sizeof(uint32_t))
#define FDT_V2_SIZE	(FDT_V1_SIZE + sizeof(uint32_t))
#define FDT_V3_SIZE	(FDT_V2_SIZE + sizeof(uint32_t))
#define FDT_V16_SIZE	FDT_V3_SIZE
#define FDT_V17_SIZE	(FDT_V16_SIZE + sizeof(uint32_t))

#endif /* _FDT_H */
