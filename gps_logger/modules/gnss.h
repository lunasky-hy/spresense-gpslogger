#define CONFIG_GNSS_DEVNAME "/dev/gps"
#define CONFIG_GNSS_ADDON_DEVNAME "/dev/gps2"
#define GNSS_POLL_FD_NUM 1
#define GNSS_POLL_TIMEOUT_FOREVER -1
#define MY_GNSS_SIG 18

struct cxd56_gnss_dms_s;
static void double_to_dmf(double x, struct cxd56_gnss_dms_s *dmf);
static int read_and_print(int fd);
static int gnss_setparams(int fd);

extern void gnss_finalize(int fd, sigset_t *mask);
extern int gnss_initialize(sigset_t *mask);
extern int gnss_get(int fd, sigset_t *mask);
extern int gnss_stop(int fd);