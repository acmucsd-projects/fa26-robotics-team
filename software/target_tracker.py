import math


class TargetTracker:
    def __init__(self, max_distance=100):
        self.target_box = None
        self.center_point = None
        self.max_distance = max_distance
        self.missing_frames = 0

    def get_center(self, box):
        x1, y1, x2, y2 = box
        return (
            int((x1 + x2) / 2),
            int((y1 + y2) / 2)
        )

    def point_inside_box(self, point, box):
        x, y = point
        x1, y1, x2, y2 = box
        return x1 <= x <= x2 and y1 <= y <= y2

    def calculate_iou(self, box1, box2):
        x1 = max(box1[0], box2[0])
        y1 = max(box1[1], box2[1])
        x2 = min(box1[2], box2[2])
        y2 = min(box1[3], box2[3])

        intersection = max(0, x2-x1) * max(0, y2-y1)

        area1 = max(0, box1[2]-box1[0]) * max(
            0, box1[3]-box1[1]
        )
        area2 = max(0, box2[2]-box2[0]) * max(
            0, box2[3]-box2[1]
        )

        union = area1 + area2 - intersection

        return intersection / union if union > 0 else 0

    def lock_target(self, detections, hand_point):
        if hand_point is None:
            return None

        candidates = []

        for box in detections.xyxy:
            if self.point_inside_box(hand_point, box):
                candidates.append(box)

        if not candidates:
            return None

        # Prefer biggest containing box if boxes overlap
        box = max(
            candidates,
            key=lambda b: (b[2]-b[0]) * (b[3]-b[1])
        )

        self.target_box = box.copy()
        self.center_point = self.get_center(box)
        self.missing_frames = 0

        print("[TRACKER] Target locked.")
        return self.center_point

    def update(self, detections):
        if self.target_box is None:
            return None

        previous_center = self.get_center(self.target_box)

        best_box = None
        best_score = float("-inf")

        prev_width = self.target_box[2] - self.target_box[0]
        prev_height = self.target_box[3] - self.target_box[1]
        prev_area = max(1, prev_width * prev_height)

        for box in detections.xyxy:
            center = self.get_center(box)

            distance = math.dist(previous_center, center)

            if distance > self.max_distance:
                continue

            iou = self.calculate_iou(self.target_box, box)

            width = box[2] - box[0]
            height = box[3] - box[1]
            area = max(1, width * height)

            size_ratio = min(area, prev_area) / max(
                area, prev_area
            )

            if size_ratio < 0.4:
                continue

            score = (
                iou
                - 0.5 * (distance / self.max_distance)
                + 0.2 * size_ratio
            )

            if score > best_score:
                best_score = score
                best_box = box

        if best_box is not None:
            self.target_box = best_box.copy()
            self.center_point = self.get_center(best_box)
            self.missing_frames = 0

            return self.center_point

        # No sufficiently close detection
        self.missing_frames += 1
        # self.center_point = None

        if self.missing_frames >= 30:
            print("[TRACKER] Target lost.")
            self.reset()

        return None

    def relock_target(self, detections, previous_center):
        if previous_center is None:
            return None

        best_box = None
        best_distance = float("inf")

        for box in detections.xyxy:
            center = self.get_center(box)

            distance = math.dist(previous_center, center)

            if distance > self.max_distance:
                continue

            if distance < best_distance:
                best_distance = distance
                best_box = box

        if best_box is not None:
            self.target_box = best_box.copy()
            self.center_point = self.get_center(best_box)
            self.missing_frames = 0

            print("[TRACKER] Target relocked.")
            return self.center_point

        # No sufficiently close detection
        self.missing_frames += 1

        if self.missing_frames >= 30:
            print("[TRACKER] Target lost.")
            self.reset()

        return None

    def reset(self):
        self.target_box = None
        # self.center_point = None
        self.missing_frames = 0
