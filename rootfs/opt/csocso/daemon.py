import SL030
import time
import config


def play(gpio, pattern):
    if len(pattern) % 2 is not 0:
        raise Exception('Pattern must contain even number of timestamps')

    if not isinstance(gpio, list):
        gpio = [gpio]

    on = False
    previous = 0
    for i in range(len(pattern)):
        time.sleep(pattern[i] - previous)
        on = not on
        for fd in gpio:
            fd.write('1' if on else '0')
        previous = pattern[i]

buzzer = SL030.open_gpio(18, 'out')
success_pattern = [0, config.magnet_duration]
#failure_pattern = [0, 0.1, 0.2, 0.3, 0.4, 0.5]

magnet = SL030.open_gpio(11, 'out')

reader = SL030.SL030(1, 0x50, 4, None)

while True:
    print map(ord, reader.poll()[1])
    play([buzzer, magnet], success_pattern)
    # play(buzzer, failure_pattern)
