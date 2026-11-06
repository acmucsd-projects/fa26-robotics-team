import os
from rfdetr import RFDETR

def main():
    MODEL_PATH = os.path.abspath("checkpoint_best_total (3).pth")

    model = RFDETR.from_checkpoint(MODEL_PATH)


    metrics = model.evaluate(
        dataset_dir="C:\\Users\\warna\\Downloads\\Autonomous Gimbal Project.v3-1000-with-nulls.yolov8",
        split="test" 
    )

    print(f"mAP50: {metrics['test/mAP_50']}")
    print(f"mAP50-95: {metrics['test/mAP_50_95']}")

if __name__ == '__main__':
    main()