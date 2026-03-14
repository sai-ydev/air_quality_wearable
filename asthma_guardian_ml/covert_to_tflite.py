import tensorflow as tf
import numpy as np
import joblib
from sklearn.model_selection import train_test_split
import pandas as pd

# Load data and split
df = pd.read_csv('sensor_log_all_three.csv')
feature_cols = ['temperature', 'humidity', 'pressure', 'iaq', 'iaq_accuracy',
                'static_iaq', 'co2_equiv', 'breath_voc', 'gas_resistance',
                'o3_ppb', 'no2_ppb', 'oaq_index', 
                'rmox_0', 'rmox_1', 'rmox_2', 'rmox_3']

X = df[feature_cols]
y = df['label']

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=y
)

# Load scaler
scaler = joblib.load('scaler.pkl')
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Build simple neural network
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(16,)),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dropout(0.3),
    tf.keras.layers.Dense(16, activation='relu'),
    tf.keras.layers.Dense(3, activation='softmax')  # 3 classes
])

model.compile(
    optimizer='adam',
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

print("Training TensorFlow model...")
history = model.fit(
    X_train_scaled, y_train,
    validation_split=0.2,
    epochs=50,
    batch_size=32,
    verbose=1
)

# Evaluate
test_loss, test_acc = model.evaluate(X_test_scaled, y_test, verbose=0)
print(f"\nTest accuracy: {test_acc:.4f}")

# Convert to TensorFlow Lite
print("\nConverting to TensorFlow Lite...")
converter = tf.lite.TFLiteConverter.from_keras_model(model)

tflite_model = converter.convert()

# Save model
with open('model.tflite', 'wb') as f:
    f.write(tflite_model)

print(f"Model saved: {len(tflite_model)} bytes")
print("Saved to: model.tflite")

# Also save scaler parameters for ESP32
scaler_mean = scaler.mean_
scaler_scale = scaler.scale_

print("\n=== Scaler Parameters (for ESP32) ===")
print("Mean:", scaler_mean.tolist())
print("Scale:", scaler_scale.tolist())

# Save as C header
with open('model_params.h', 'w') as f:
    f.write("// Auto-generated scaler parameters\n")
    f.write("#pragma once\n\n")
    f.write("const float SCALER_MEAN[] = {")
    f.write(", ".join([f"{x}f" for x in scaler_mean]))
    f.write("};\n\n")
    f.write("const float SCALER_SCALE[] = {")
    f.write(", ".join([f"{x}f" for x in scaler_scale]))
    f.write("};\n")

print("Scaler saved to model_params.h")