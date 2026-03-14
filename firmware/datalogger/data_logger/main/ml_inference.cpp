#include "ml_inference.h"
#include "model_data.h"
#include "model_params.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ml";

// TFLite globals
static const tflite::Model* model = nullptr;
static tflite::MicroInterpreter* interpreter = nullptr;
static TfLiteTensor* input = nullptr;
static TfLiteTensor* output = nullptr;

// Memory arena for TFLite (adjust size if needed)
constexpr int kTensorArenaSize = 40 * 1024;  // 40KB
static uint8_t tensor_arena[kTensorArenaSize];

// Feature order (must match training!)
static const char* feature_names[] = {
    "temperature", "humidity", "pressure", "iaq", "iaq_accuracy",
    "static_iaq", "co2_equiv", "breath_voc", "gas_resistance",
    "o3_ppb", "no2_ppb", "oaq_index",
    "rmox_0", "rmox_1", "rmox_2", "rmox_3"
};

// Class names
static const char* class_names[] = {"LOW", "MODERATE", "HIGH"};

esp_err_t ml_inference_init(void)
{
    ESP_LOGI(TAG, "Initializing TensorFlow Lite model");
    
    // Load model
    model = tflite::GetModel(model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG, "Model schema mismatch!");
        return ESP_FAIL;
    }
    
    // Set up operator resolver (add operations your model uses)
    static tflite::MicroMutableOpResolver<10> resolver;
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver.AddRelu();
    
    // Build interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;
    
    // Allocate tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        ESP_LOGE(TAG, "AllocateTensors failed");
        return ESP_FAIL;
    }
    
    // Get input/output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    ESP_LOGI(TAG, "Model loaded successfully");
    ESP_LOGI(TAG, "  Input shape: [%d, %d]", input->dims->data[0], input->dims->data[1]);
    ESP_LOGI(TAG, "  Output shape: [%d, %d]", output->dims->data[0], output->dims->data[1]);
    ESP_LOGI(TAG, "  Arena used: %d / %d bytes", 
             interpreter->arena_used_bytes(), kTensorArenaSize);
    
    return ESP_OK;
}

int8_t ml_inference_predict(const sensor_reading_t *reading, float *confidence)
{
    if (!interpreter || !input || !output) {
        ESP_LOGE(TAG, "Model not initialized");
        return -1;
    }
    
    // Extract features in correct order
    float features[16] = {
        reading->temperature,
        reading->humidity,
        reading->pressure,
        reading->iaq,
        (float)reading->iaq_accuracy,
        reading->static_iaq,
        reading->co2_equivalent,
        reading->breath_voc,
        reading->gas_resistance,
        reading->o3_ppb,
        reading->no2_ppb,
        reading->oaq_index,
        reading->rmox_0,
        reading->rmox_1,
        reading->rmox_2,
        reading->rmox_3
    };
    
    // Apply scaling (StandardScaler from training)
    for (int i = 0; i < 16; i++) {
        features[i] = (features[i] - SCALER_MEAN[i]) / SCALER_SCALE[i];
    }
    
    // Copy to input tensor
    for (int i = 0; i < 16; i++) {
        input->data.f[i] = features[i];
    }
    
    // Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        ESP_LOGE(TAG, "Inference failed");
        return -1;
    }
    
    // Get prediction (class with highest probability)
    int8_t predicted_class = 0;
    float max_prob = output->data.f[0];
    
    for (int i = 1; i < 3; i++) {
        if (output->data.f[i] > max_prob) {
            max_prob = output->data.f[i];
            predicted_class = i;
        }
    }
    
    if (confidence) {
        *confidence = max_prob;
    }
    
    ESP_LOGI(TAG, "Prediction: %s (%.1f%% confidence)", 
             class_names[predicted_class], max_prob * 100);
    
    return predicted_class;
}

const char* ml_inference_get_class_name(int8_t class_id)
{
    if (class_id >= 0 && class_id < 3) {
        return class_names[class_id];
    }
    return "UNKNOWN";
}