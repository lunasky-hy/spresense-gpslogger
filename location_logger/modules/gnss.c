#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>
#include <arch/chip/gnss.h>
#include "gnss.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cxd56_gnss_dms_s
{
  int8_t sign;
  uint8_t degree;
  uint8_t minute;
  uint32_t frac;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint32_t posfixflag;
static struct cxd56_gnss_positiondata_s posdat;
struct cxd56_gnss_signal_setting_s setting;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: double_to_dmf()
 *
 * Description:
 *   Convert from double format to degree-minute-frac format.
 *
 * Input Parameters:
 *   x   - double value.
 *   dmf - Address to store the conversion result.
 *
 * Returned Value:
 *   none.
 *
 * Assumptions/Limitations:
 *   none.
 *
 ****************************************************************************/

void double_to_dmf(double x, struct cxd56_gnss_dms_s *dmf)
{
  int b;
  int d;
  int m;
  double f;
  double t;

  if (x < 0)
  {
    b = 1;
    x = -x;
  }
  else
  {
    b = 0;
  }
  d = (int)x; /* = floor(x), x is always positive */
  t = (x - d) * 60;
  m = (int)t; /* = floor(t), t is always positive */
  f = (t - m) * 10000;

  dmf->sign = b;
  dmf->degree = d;
  dmf->minute = m;
  dmf->frac = f;
}

/****************************************************************************
 * Name: read_and_print()
 *
 * Description:
 *   Read and print POS data.
 *
 * Input Parameters:
 *   fd - File descriptor.
 *
 * Returned Value:
 *   Zero (OK) on success; Negative value on error.
 *
 * Assumptions/Limitations:
 *   none.
 *
 ****************************************************************************/

int read_and_print(int fd)
{
  int ret;
  struct cxd56_gnss_dms_s dmf;

  /* Read POS data. */

  ret = read(fd, &posdat, sizeof(posdat));
  if (ret < 0)
  {
    printf("read error\n");
    goto _err;
  }
  else if (ret != sizeof(posdat))
  {
    ret = ERROR;
    printf("read size error\n");
    goto _err;
  }
  else
  {
    ret = OK;
  }

  /* Print POS data. */

  /* Print time. */

  printf(">Hour:%d, minute:%d, sec:%d, usec:%ld\n",
         posdat.receiver.time.hour, posdat.receiver.time.minute,
         posdat.receiver.time.sec, posdat.receiver.time.usec);
  if (posdat.receiver.pos_fixmode != CXD56_GNSS_PVT_POSFIX_INVALID)
  {
    /* 2D fix or 3D fix.
     * Convert latitude and longitude into dmf format and print it. */

    posfixflag = 1;

    double_to_dmf(posdat.receiver.latitude, &dmf);
    printf(">LAT %d.%d.%04ld\n", dmf.degree, dmf.minute, dmf.frac);

    double_to_dmf(posdat.receiver.longitude, &dmf);
    printf(">LNG %d.%d.%04ld\n", dmf.degree, dmf.minute, dmf.frac);
  }
  else
  {
    /* No measurement. */

    printf(">No Positioning Data\n");
  }

_err:
  return ret;
}

/****************************************************************************
 * Name: gnss_setparams()
 *
 * Description:
 *   Set gnss parameters use ioctl.
 *
 * Input Parameters:
 *   fd - File descriptor.
 *
 * Returned Value:
 *   Zero (OK) on success; Negative value on error.
 *
 * Assumptions/Limitations:
 *   none.
 *
 ****************************************************************************/

int gnss_setparams(int fd)
{
  int ret = 0;
  uint32_t set_satellite;
  struct cxd56_gnss_ope_mode_param_s set_opemode;

  /* Set the GNSS operation interval. */

  set_opemode.mode = 1;     /* Operation mode:Normal(default). */
  set_opemode.cycle = 1000; /* Position notify cycle(msec step). */

  ret = ioctl(fd, CXD56_GNSS_IOCTL_SET_OPE_MODE, (uint32_t)&set_opemode);
  if (ret < 0)
  {
    printf("ioctl(CXD56_GNSS_IOCTL_SET_OPE_MODE) NG!!\n");
    goto _err;
  }

  /* Set the type of satellite system used by GNSS. */

  set_satellite = CXD56_GNSS_SAT_GPS | CXD56_GNSS_SAT_GLONASS;

  ret = ioctl(fd, CXD56_GNSS_IOCTL_SELECT_SATELLITE_SYSTEM, set_satellite);
  if (ret < 0)
  {
    printf("ioctl(CXD56_GNSS_IOCTL_SELECT_SATELLITE_SYSTEM) NG!!\n");
    goto _err;
  }

_err:
  return ret;
}

/****************************************************************************
 * Name: gnss_finalize()
 *
 * Description:
 *  Finalize GNSS.
 *  Use this function to stop GNSS and release resources, when GNSS is no longer needed.
 *  This function should be called after gnss_init().
 *
 * Input Parameters:
 *  fd      - File descriptor.
 *  setting - Signal setting.
 *  mask    - Signal mask.
 *
 * Returned Value:
 *  None.
 *
 ****************************************************************************/

void gnss_finalize(int fd, sigset_t *mask)
{
  setting.enable = 0;
  int ret = ioctl(fd, CXD56_GNSS_IOCTL_SIGNAL_SET, (unsigned long)&setting);
  if (ret < 0)
  {
    printf("signal error\n");
  }

  sigprocmask(SIG_UNBLOCK, mask, NULL);

  /* Release GNSS file descriptor. */
  ret = close(fd);
}

/****************************************************************************
 * Name: gnss_init()
 *
 * description:
 *  Initialize GNSS.
 *
 * Input Parameters:
 *  none.
 *
 * Returned Value:
 *  Zero (OK) on success; Negative value on error.
 ****************************************************************************/

int gnss_initialize(sigset_t *mask)
{

  int fd;
  int ret;

  /* Get file descriptor to control GNSS. */

  fd = open(CONFIG_GNSS_ADDON_DEVNAME, O_RDONLY);
  if (fd < 0)
  {
    printf("open error:%d,%d\n", fd, errno);
    return -ENODEV;
  }

  /* Configure mask to notify GNSS signal. */

  sigemptyset(mask);
  sigaddset(mask, MY_GNSS_SIG);
  ret = sigprocmask(SIG_BLOCK, mask, NULL);
  if (ret != OK)
  {
    printf("sigprocmask failed. %d\n", ret);
    gnss_finalize(fd, mask);
    return -1;
  }

  /* Set the signal to notify GNSS events. */

  setting.fd = fd;
  setting.enable = 1;
  setting.gnsssig = CXD56_GNSS_SIG_GNSS;
  setting.signo = MY_GNSS_SIG;
  setting.data = NULL;

  ret = ioctl(fd, CXD56_GNSS_IOCTL_SIGNAL_SET, (unsigned long)&setting);
  if (ret < 0)
  {
    printf("signal error\n");
    gnss_finalize(fd, mask);
    return ret;
  }

  /* Set GNSS parameters. */

  ret = gnss_setparams(fd);
  if (ret != OK)
  {
    printf("gnss_setparams failed. %d\n", ret);
    gnss_finalize(fd, mask);
    return -1;
  }

  /* Start GNSS. */

  ret = ioctl(fd, CXD56_GNSS_IOCTL_START, CXD56_GNSS_STMOD_HOT);
  if (ret < 0)
  {
    printf("start GNSS ERROR %d\n", errno);
    gnss_finalize(fd, mask);
    return -1;
  }
  else
  {
    printf("start GNSS OK\n");
  }

  return fd;
}

int gnss_get(int fd, sigset_t *mask, struct gnss_positiondata_s *position_data)
{
  int ret;

  ret = sigwaitinfo(mask, NULL);
  if (ret != MY_GNSS_SIG)
  {
    printf("sigwaitinfo error %d\n", ret);
    return -1;
  }

  /* Read POS data. */
  ret = read(fd, &posdat, sizeof(posdat));
  if (ret < 0)
  {
    printf("read error\n");
    return ret;
  }
  else if (ret != sizeof(posdat))
  {
    ret = ERROR;
    printf("read size error\n");
    return ret;
  }

  if (posdat.receiver.pos_fixmode != CXD56_GNSS_PVT_POSFIX_INVALID)
  {
    position_data->latitude = posdat.receiver.latitude;
    position_data->longitude = posdat.receiver.longitude;
    position_data->altitude = posdat.receiver.altitude;
    position_data->velocity = posdat.receiver.velocity;
    position_data->direction = posdat.receiver.direction;

    return OK;
  }
  else
  {
    return 1;
  }
}

int gnss_stop(int fd)
{
  int ret;

  /* Stop GNSS. */

  ret = ioctl(fd, CXD56_GNSS_IOCTL_STOP, 0);
  if (ret < 0)
  {
    printf("stop GNSS ERROR\n");
  }
  else
  {
    printf("stop GNSS OK\n");
  }

  return ret;
}

int gnss_first_contact(int fd, sigset_t *mask)
{
  int ret;
  
  do {
    printf("GNSS first position contact...\n");
    ret = sigwaitinfo(mask, NULL);

    if (ret != MY_GNSS_SIG)
    {
      printf("sigwaitinfo error %d\n", ret);
      return -1;
    }

    /* Read POS data. */
    ret = read(fd, &posdat, sizeof(posdat));
    if (ret < 0)
    {
      printf("read error\n");
      return ret;
    }
    else if (ret != sizeof(posdat))
    {
      ret = ERROR;
      printf("read size error\n");
      return ret;
    }

  } while (posdat.receiver.pos_fixmode == CXD56_GNSS_PVT_POSFIX_INVALID);

  return OK;
}