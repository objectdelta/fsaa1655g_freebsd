#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>

#define DRIVER_NAME "radio"

#define BUFFER_SIZE 2


#define KBD_STATE_PORT 0x64
#define KBD_COMMAND_PORT 0x64
#define KBD_DATA_PORT 0x60

#define WIFI_COMMAND 0xC5
#define WIFI_ON 0x25
#define WIFI_OFF 0x45

static d_open_t radio_open;
static d_close_t radio_close;
static d_read_t radio_read;
static d_write_t radio_write;

static struct cdevsw radio_cdevsw = {
  .d_version = D_VERSION,
  .d_open = radio_open,
  .d_close = radio_close,
  .d_read = radio_read,
  .d_write = radio_write,
  .d_name = DRIVER_NAME
};

typedef struct radio {
  char buffer[BUFFER_SIZE];
  int length;
} radio_t; 


// static int radio_state = 0;
static radio_t *radio_msg;
static int radio_state = 0;

static struct cdev *radio_dev;


static void set_radio(int state)
{
  unsigned char val = 0;
  if (radio_state != state)
    {
      do { val = inb(KBD_STATE_PORT); } while ((val & 2) == 2);
      outb(KBD_COMMAND_PORT, WIFI_COMMAND);
      do { val = inb(KBD_STATE_PORT); } while ((val & 2) == 2);
	  
      if (state == 1)
	{
	  outb(KBD_DATA_PORT, WIFI_ON);
	  radio_state = 1;
	  uprintf("Radio turned on.\n");
	}
      else
	{
	  outb(KBD_DATA_PORT, WIFI_OFF);
	  radio_state = 0;
	  uprintf("Radio turned off.\n");
	}
    }
}


static int radio_open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
  //uprintf("Opening radio device...\n");
  return (0);
}

static int radio_close(struct cdev *dev, int fflag, int dev_type, struct thread *td)
{
  //uprintf("Closing radio device.\n");
  return (0);
}

static int radio_write(struct cdev *dev, struct uio *uio, int ioflag)
{
  int error = 0;
  int state = 0;

  error = copyin(uio->uio_iov->iov_base, radio_msg->buffer, MIN(uio->uio_iov->iov_len, BUFFER_SIZE -1));
  if (error != 0)
    {
      uprintf("Write failed!\n");
      return (error);
    }
  *(radio_msg->buffer + MIN(uio->uio_iov->iov_len, BUFFER_SIZE -1)) = 0;
  radio_msg->length = MIN(uio->uio_iov->iov_len, BUFFER_SIZE -1);

  state = 0;
  if (radio_msg->buffer[0] == '1')
    state = 1;

  set_radio(state);

  return (error);
}

static int radio_read(struct cdev *dev, struct uio *uio, int ioflag)
{
  int error = 0;
  int amount = 0;
  radio_msg->buffer[0] = (radio_state != 0) ? '1' : '0';
  radio_msg->buffer[1] = (char)0;

  amount = MIN(uio->uio_resid, (2 - uio->uio_offset > 0) ? 2 - uio->uio_offset : 0);
  error = uiomove(radio_msg->buffer + uio->uio_offset, amount, uio);
  if (error != 0)
    {
      uprintf("Read failed!\n");
    }
  return (error);
}

static int radio_modevent(module_t mod __unused, int event, void *argv __unused)
{
  int error = 0;

  switch(event)
    {
    case MOD_LOAD:
      radio_msg = malloc(sizeof(radio_t), M_TEMP, M_WAITOK);
      radio_dev = make_dev(&radio_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600, "radio");
      set_radio(1);
      uprintf("Radio driver loaded.\n");
      break;
    case MOD_UNLOAD:
      destroy_dev(radio_dev);
      free(radio_msg, M_TEMP);
      uprintf("Radio driver unloaded.\n");
      break;
    default:
      error = EOPNOTSUPP;
      break;
    }

  return (error);
}

DEV_MODULE(radio, radio_modevent, NULL);
