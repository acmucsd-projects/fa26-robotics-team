"""Convert full-frame camera pixels to relative gimbal pointing angles."""

import math


class GimbalKinematics:
    """Pinhole camera with centered principal point and negligible distortion."""

    def __init__(self, horizontal_fov_deg, vertical_fov_deg):
        self.horizontal_half_fov_tan = self._half_fov_tan(horizontal_fov_deg)
        self.vertical_half_fov_tan = (
            None if vertical_fov_deg is None else self._half_fov_tan(vertical_fov_deg)
        )

    @staticmethod
    def _half_fov_tan(fov_deg):
        if not 0.0 < fov_deg < 180.0:
            raise ValueError("Camera field of view must be between 0 and 180 degrees.")
        return math.tan(math.radians(fov_deg / 2.0))

    def angles_for_pixel(self, center, width, height):
        """Return (yaw, pitch) in degrees, positive right and up."""
        if width <= 0 or height <= 0:
            raise ValueError("Frame dimensions must be positive.")

        cx, cy = width / 2.0, height / 2.0
        fx = cx / self.horizontal_half_fov_tan
        # With only the horizontal FOV, assume square pixels.
        fy = fx if self.vertical_half_fov_tan is None else cy / self.vertical_half_fov_tan

        u, v = center
        x = (u - cx) / fx
        y = (cy - v) / fy
        yaw = math.atan2(x, 1.0)
        pitch = math.atan2(y, math.hypot(x, 1.0))
        return (math.degrees(yaw), math.degrees(pitch))