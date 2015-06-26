import evdev

class Input:
    def __init__(self):
        self.ui = evdev.UInput()

    def inject(self, events):
        for e in events:
            self.ui.write(e[0], e[1], e[2])
        self.ui.syn()

