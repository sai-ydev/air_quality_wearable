import os

def convert_to_c_array(tflite_path, output_path):
    with open(tflite_path, 'rb') as f:
        model_data = f.read()
    
    with open(output_path, 'w') as f:
        f.write('#pragma once\n\n')
        f.write('// Auto-generated TFLite model\n')
        f.write(f'const unsigned int model_tflite_len = {len(model_data)};\n')
        f.write('const unsigned char model_tflite[] = {\n  ')
        
        hex_values = [f'0x{b:02x}' for b in model_data]
        for i, val in enumerate(hex_values):
            f.write(val)
            if i < len(hex_values) - 1:
                f.write(',')
                if (i + 1) % 12 == 0:
                    f.write('\n  ')
                else:
                    f.write(' ')
        
        f.write('\n};\n')
    
    print(f'Model converted: {len(model_data)} bytes')
    print(f'Saved to: {output_path}')

convert_to_c_array('model.tflite', 'model_data.h')