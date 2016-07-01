#ifndef DEFINE_H
#define DEFINE_H
#define MBYTE(x)					((x)*(1024)*(1024))
#define KBYTE(x)					((x)*(1024))

#define FIRST_BOOT_INFO_ADDR        (0x41000)
#define FIRST_BOOT_INFO_LENGTH      (KBYTE(4))

#define SECTION0_INFO_ADDR          (FIRST_BOOT_INFO_ADDR + FIRST_BOOT_INFO_LENGTH)
#define SECTION0_INFO_LENGTH        (KBYTE(4))

#define SECTION0_ADDR				(0x49000)
#define SECTION0_LENGTH             (KBYTE(692))

#define PQ_ADDR						(0x1A3000)
#define PQ_LENGTH_2M				(10*KBYTE(20))
#define PQ_LENGTH_4M				(30*KBYTE(20))

#define USER_ADDR(x)				(MBYTE(x) - RO_SECTION_LENGTH - USER_LENGTH)
#define USER_LENGTH                 (KBYTE(8))

#define RO_SECTION_LENGTH			(KBYTE(64))

#define UNIT_LENGTH                 (KBYTE(64))

#define ALL_BLOCKS					(4)

#define ALL_LENGTH_2M				(SECTION0_INFO_LENGTH + SECTION0_LENGTH + PQ_LENGTH_2M + USER_LENGTH)
#define ALL_LENGTH_4M				(SECTION0_INFO_LENGTH + SECTION0_LENGTH + PQ_LENGTH_4M + USER_LENGTH)
#endif // DEFINE_H
