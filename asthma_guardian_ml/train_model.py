import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import classification_report, confusion_matrix
import xgboost as xgb
import matplotlib.pyplot as plt
import seaborn as sns
import joblib

# Load data
print("Loading data...")
df = pd.read_csv('sensor_log_all_three.csv')

print(f"Total samples: {len(df)}")
print(f"\nLabel distribution:")
print(df['label'].value_counts().sort_index())

# Select features (exclude timestamp and event_name)
feature_cols = ['temperature', 'humidity', 'pressure', 'iaq', 'iaq_accuracy',
                'static_iaq', 'co2_equiv', 'breath_voc', 'gas_resistance',
                'o3_ppb', 'no2_ppb', 'oaq_index', 
                'rmox_0', 'rmox_1', 'rmox_2', 'rmox_3']

X = df[feature_cols]
y = df['label']

print(f"\nFeatures shape: {X.shape}")
print(f"Target shape: {y.shape}")

# Check for missing values
print(f"\nMissing values:\n{X.isnull().sum()}")

# Split data: 80% train, 20% test
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=42, stratify=y
)

print(f"\nTrain samples: {len(X_train)}")
print(f"Test samples: {len(X_test)}")

# Scale features
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# Train XGBoost model
print("\nTraining XGBoost model...")
model = xgb.XGBClassifier(
    n_estimators=100,
    max_depth=6,
    learning_rate=0.1,
    random_state=42,
    eval_metric='mlogloss'
)

model.fit(X_train_scaled, y_train)

# Evaluate
y_pred = model.predict(X_test_scaled)

print("\n=== Model Performance ===")
print("\nClassification Report:")
print(classification_report(y_test, y_pred, 
                            target_names=['LOW', 'MODERATE', 'HIGH']))

# Confusion matrix
cm = confusion_matrix(y_test, y_pred)
plt.figure(figsize=(8, 6))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues',
            xticklabels=['LOW', 'MODERATE', 'HIGH'],
            yticklabels=['LOW', 'MODERATE', 'HIGH'])
plt.title('Confusion Matrix')
plt.ylabel('True Label')
plt.xlabel('Predicted Label')
plt.savefig('confusion_matrix.png')
print("\nConfusion matrix saved to confusion_matrix.png")

# Feature importance
feature_importance = pd.DataFrame({
    'feature': feature_cols,
    'importance': model.feature_importances_
}).sort_values('importance', ascending=False)

print("\n=== Feature Importance ===")
print(feature_importance)

# Plot feature importance
plt.figure(figsize=(10, 8))
plt.barh(feature_importance['feature'], feature_importance['importance'])
plt.xlabel('Importance')
plt.title('Feature Importance')
plt.tight_layout()
plt.savefig('feature_importance.png')
print("Feature importance saved to feature_importance.png")

# Save model and scaler
joblib.dump(model, 'xgboost_model.pkl')
joblib.dump(scaler, 'scaler.pkl')
print("\nModel saved to xgboost_model.pkl")
print("Scaler saved to scaler.pkl")

# Calculate accuracy
accuracy = (y_pred == y_test).mean()
print(f"\nOverall Accuracy: {accuracy:.2%}")