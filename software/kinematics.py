import math

class PinholeCamera:
    """Pixel -> (d_pan, d_tilt) in degrees. +pan = right, +tilt = up."""

    def __init__(self, width, height, hfov_deg=70.0):
        self.cx, self.cy = width / 2, height / 2
        self.fx = self.cx / math.tan(math.radians(hfov_deg) / 2)
        self.fy = self.fx

    def pixel_to_angles(self, u, v):
        x = (u - self.cx) / self.fx
        y = (v - self.cy) / self.fy
        d_pan = math.atan2(x, 1.0)
        d_tilt = math.atan2(-y, math.hypot(1.0, x))
        return math.degrees(d_pan), math.degrees(d_tilt)