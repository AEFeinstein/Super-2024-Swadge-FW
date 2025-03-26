from enum import Enum
from random import randint
# class syntax


class swadgeState(Enum):
    SLEEPING = 1
    WAKING = 2
    TRANSMITTING = 3
    LISTENING = 4
    ENTERING_SLEEP = 5


class swadgeSim:
    def __init__(self, listenMs: int, minSleepMs: int, maxSleepMs: int):
        self.state: swadgeState = swadgeState.SLEEPING
        self.clockMs = 0
        self.transitionTimer = 1
        self.listenMs = listenMs
        self.minSleepMs = minSleepMs
        self.maxSleepMs = maxSleepMs

    def simTimeStep(self):
        # Step the ms clock
        self.clockMs += 1

        # Check for transitions
        self.transitionTimer -= 1
        if 0 == self.transitionTimer:
            match self.state:
                case swadgeState.SLEEPING:
                    # Go from sleeping to waking for 2ms
                    self.state = swadgeState.WAKING
                    self.transitionTimer = 2
                    pass
                case swadgeState.WAKING:
                    # Go from sleeping to transmitting for 1ms
                    self.state = swadgeState.TRANSMITTING
                    self.transitionTimer = 1
                    pass
                case swadgeState.TRANSMITTING:
                    # Go from transmitting to listening for self.listenMs
                    self.state = swadgeState.LISTENING
                    self.transitionTimer = self.listenMs
                    pass
                case swadgeState.LISTENING:
                    # Go from listening to entering sleep for 1ms
                    self.state = swadgeState.ENTERING_SLEEP
                    self.transitionTimer = 1
                    pass
                case swadgeState.ENTERING_SLEEP:
                    # Go from transmitting to listening for random time
                    self.state = swadgeState.SLEEPING
                    self.transitionTimer = randint(
                        self.minSleepMs, self.maxSleepMs)
                    pass

    def getState(self) -> swadgeState:
        return self.state

    def goToSleep(self):
        self.transitionTimer = 1
        self.state = swadgeState.LISTENING


swadges: list[swadgeSim] = []
for _ in range(3):
    swadges.append(swadgeSim(listenMs=1000, minSleepMs=5000, maxSleepMs=15000))

passCount = 0

# For every millisecond in a day
for ms in range(24 * 60 * 60 * 1000):

    # For each swadge, simulate this ms
    for swadge in swadges:
        swadge.simTimeStep()

    # For each combination of swadges, check if they hear each other
    for srcIdx in range(len(swadges)):
        for dstIdx in range(srcIdx + 1, len(swadges)):
            if ((swadgeState.TRANSMITTING == swadges[srcIdx].getState()) and (swadgeState.LISTENING == swadges[dstIdx].getState())) or \
                    ((swadgeState.TRANSMITTING == swadges[dstIdx].getState()) and (swadgeState.LISTENING == swadges[srcIdx].getState())):
                # print('%d -> %d' % (srcIdx, dstIdx))
                # swadges[srcIdx].goToSleep()
                # swadges[dstIdx].goToSleep()
                passCount += 1

print('%d passes, %f per hour' % (passCount, passCount / 24))