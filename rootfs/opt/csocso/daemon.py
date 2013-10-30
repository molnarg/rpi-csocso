import SL030
import time
import config
import urllib


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
failure_pattern = [0, 0.1, 0.2, 0.3, 0.4, 0.5]

magnet = SL030.open_gpio(11, 'out')

reader = SL030.SL030(1, 0x50, 4, None)

while True:
    card = reader.poll()
    if card:
        card_no = card[1].encode('hex_codec')
        print 'Card: %s' % card_no

        # Accept or reject
        if True:
            play([buzzer, magnet], success_pattern)
        else:
            play(buzzer, failure_pattern)

        # Logging:
        log_url = config.log_url % card_no
        try:
            urllib.urlopen(log_url)
        except Exception as e:
            print e
            print 'Could not send HTTP request to %s' % log_url
