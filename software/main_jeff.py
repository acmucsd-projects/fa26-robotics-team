import os
from rfdetr import RFDETR

def main():
    MODEL_PATH = os.path.abspath("checkpoint_best_total.pth")

    model = RFDETR.from_checkpoint(MODEL_PATH)


    metrics = model.evaluate(
        dataset_dir="C:\\Users\\jeffe\\Downloads\\robo\\Autonomous Gimbal Project.v2-1000-image-dataset.yolov8", 
        split="test" 
    )

    print(f"mAP50: {metrics['test/mAP_50']}")
    print(f"mAP50-95: {metrics['test/mAP_50_95']}")

if __name__ == '__main__':
    main()