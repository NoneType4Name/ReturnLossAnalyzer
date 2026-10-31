.section .rdata,"dr"

.global _binary_freq_bin_start
.global __binary_freq_bin_start
.global _binary_freq_bin_end
.global __binary_freq_bin_end

.global _binary_mag_db_bin_start
.global __binary_mag_db_bin_start
.global _binary_mag_db_bin_end
.global __binary_mag_db_bin_end

_binary_freq_bin_start:
__binary_freq_bin_start:
    .incbin "D:/main/coding/u/vna/dataset/freq.bin"
_binary_freq_bin_end:
__binary_freq_bin_end:

_binary_mag_db_bin_start:
__binary_mag_db_bin_start:
    .incbin "D:/main/coding/u/vna/dataset/mag_db.bin"
_binary_mag_db_bin_end:
__binary_mag_db_bin_end:
