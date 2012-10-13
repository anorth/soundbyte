import sys
import termios
import tty

def getch():
  fd = sys.stdin.fileno()
  old_settings = None
  try:
    old_settings = termios.tcgetattr(fd)
    tty.setraw(sys.stdin.fileno())
    ch = sys.stdin.read(1)
  except:
    ch = sys.stdin.read(1)
  finally:
    if old_settings is not None:
      termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
  return ch

