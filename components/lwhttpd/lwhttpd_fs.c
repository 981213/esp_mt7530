#include <errno.h>
#include <lwip/apps/httpd.h>
#include <lwip/apps/fs.h>
#include <spiffs.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_log.h>

#define LOGTAG "http_spiffs"

/*
 * Of course these functions are duplicated from esp_spiffs.c because the
 * espressif SDK told us NOTHING about the usage of their wrapper.
 */

#define SECTOR_SIZE (4 * 1024)
#define LOG_BLOCK (SECTOR_SIZE)
#define LOG_PAGE (128)

#define FD_BUF_SIZE 32 * 4
#define CACHE_BUF_SIZE (LOG_PAGE + 32) * 8

#define NUM_SYS_FD 3

static spiffs fs;

static u8_t *spiffs_work_buf;
static u8_t *spiffs_fd_buf;
static u8_t *spiffs_cache_buf;

#define FLASH_UNIT_SIZE 4

static s32_t esp_spiffs_readwrite(u32_t addr, u32_t size, u8_t *p, int write)
{
	/*
     * With proper configurarion spiffs never reads or writes more than
     * LOG_PAGE_SIZE
     */

	if (size > fs.cfg.log_page_size) {
		printf("Invalid size provided to read/write (%d)\n\r",
		       (int)size);
		return SPIFFS_ERR_NOT_CONFIGURED;
	}

	char tmp_buf[fs.cfg.log_page_size + FLASH_UNIT_SIZE * 2];
	u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
	u32_t aligned_size =
		((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) +
		FLASH_UNIT_SIZE;

	int res = spi_flash_read(aligned_addr, (u32_t *)tmp_buf, aligned_size);

	if (res != 0) {
		printf("spi_flash_read failed: %d (%d, %d)\n\r", res,
		       (int)aligned_addr, (int)aligned_size);
		return res;
	}

	if (!write) {
		memcpy(p, tmp_buf + (addr - aligned_addr), size);
		return SPIFFS_OK;
	}

	memcpy(tmp_buf + (addr - aligned_addr), p, size);

	res = spi_flash_write(aligned_addr, (u32_t *)tmp_buf, aligned_size);

	if (res != 0) {
		//	    printf("spi_flash_write failed: %d (%d, %d)\n\r", res,
		//	              (int) aligned_addr, (int) aligned_size);
		return res;
	}

	return SPIFFS_OK;
}

static s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
	return esp_spiffs_readwrite(addr, size, dst, 0);
}

static s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
	return esp_spiffs_readwrite(addr, size, src, 1);
}

static s32_t esp_spiffs_erase(u32_t addr, u32_t size)
{
	/*
     * With proper configurarion spiffs always
     * provides here sector address & sector size
     */
	if (size != fs.cfg.phys_erase_block ||
	    addr % fs.cfg.phys_erase_block != 0) {
		printf("Invalid size provided to esp_spiffs_erase (%d, %d)\n\r",
		       (int)addr, (int)size);
		return SPIFFS_ERR_NOT_CONFIGURED;
	}

	return spi_flash_erase_sector(addr / fs.cfg.phys_erase_block);
}

int httpd_spiffs_init(void)
{
	if (SPIFFS_mounted(&fs)) {
		ESP_LOGE(LOGTAG, "file system already mounted.");
		return -EBUSY;
	}

	spiffs_config cfg;
	s32_t ret;
	const esp_partition_t *fs_part_info;

	fs_part_info = esp_partition_find_first(
		ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "wwwroot");
	if (!fs_part_info) {
		ESP_LOGE(LOGTAG, "unable to find wwwroot partition");
		return -EINVAL;
	}

	cfg.phys_size = fs_part_info->size;
	cfg.phys_addr = fs_part_info->address;
	cfg.phys_erase_block = SECTOR_SIZE;
	cfg.log_block_size = LOG_BLOCK;
	cfg.log_page_size = LOG_PAGE;

	cfg.hal_read_f = esp_spiffs_read;
	cfg.hal_write_f = esp_spiffs_write;
	cfg.hal_erase_f = esp_spiffs_erase;

	if (spiffs_work_buf != NULL) {
		free(spiffs_work_buf);
		spiffs_work_buf = NULL;
	}
	spiffs_work_buf = malloc(LOG_PAGE * 2);

	if (spiffs_work_buf == NULL) {
		return -1;
	}

	if (spiffs_fd_buf != NULL) {
		free(spiffs_fd_buf);
		spiffs_fd_buf = NULL;
	}
	spiffs_fd_buf = malloc(FD_BUF_SIZE * 2);

	if (spiffs_fd_buf == NULL) {
		free(spiffs_work_buf);
		return -1;
	}

	if (spiffs_cache_buf != NULL) {
		free(spiffs_cache_buf);
		spiffs_cache_buf = NULL;
	}
	spiffs_cache_buf = malloc(CACHE_BUF_SIZE);

	if (spiffs_cache_buf == NULL) {
		free(spiffs_work_buf);
		free(spiffs_fd_buf);
		return -1;
	}

	ret = SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fd_buf,
			   FD_BUF_SIZE * 2, spiffs_cache_buf, CACHE_BUF_SIZE,
			   0);

	if (ret == -1) {
		free(spiffs_work_buf);
		free(spiffs_fd_buf);
		free(spiffs_cache_buf);
	}

	ret = SPIFFS_errno(&fs);

	return ret;
}

void httpd_spiffs_deinit(u8_t format)
{
	if (SPIFFS_mounted(&fs)) {
		SPIFFS_unmount(&fs);
		free(spiffs_work_buf);
		free(spiffs_fd_buf);
		free(spiffs_cache_buf);
	}
	if (format) {
		SPIFFS_format(&fs);
	}
}

/* be careful of the abnormal return value */
int fs_open_custom(struct fs_file *file, const char *name)
{
	spiffs_file *fdp = malloc(sizeof(spiffs_file));
	spiffs_stat f_stat;
	s32_t ret;

	ret = SPIFFS_open(&fs, name, SPIFFS_O_RDONLY, 0);
	if (ret < 0) {
		free(fdp);
		return 0;
	}
	*fdp = (spiffs_file)ret;
	file->pextension = (void *)fdp;
	SPIFFS_fstat(&fs, *fdp, &f_stat);
	file->len = f_stat.size;
	file->index = 0;
	return 1;
}
void fs_close_custom(struct fs_file *file)
{
	spiffs_file *fdp = (spiffs_file *)file->pextension;
	SPIFFS_close(&fs, *fdp);
	free(fdp);
}

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
	int ret;
	spiffs_file *fdp = (spiffs_file *)file->pextension;
	ret = SPIFFS_read(&fs, *fdp, (void *)buffer, count);
	file->index += ret;
	return ret;
}
