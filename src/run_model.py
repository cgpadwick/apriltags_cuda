# import tensorrt as trt

# with open("/home/nvidia/Downloads/note-w-false-640-640-yolov5s.engine", "rb") as f:
#     serialized_engine = f.read()

# logger = trt.Logger(trt.Logger.WARNING)
# runtime = trt.Runtime(logger)


# engine = runtime.deserialize_cuda_engine(serialized_engine)
# context = engine.create_execution_context()


import tensorrt as trt
import numpy as np
import pycuda.driver as cuda
import pycuda.autoinit

class TensorRTInference:
    def __init__(self, engine_path):
        """
        Initialize TensorRT inference engine
        Args:
            engine_path: Path to the TensorRT engine file (.engine)
        """
        self.logger = trt.Logger(trt.Logger.WARNING)
        self.runtime = trt.Runtime(self.logger)
        
        # Load engine file
        with open(engine_path, 'rb') as f:
            self.engine = self.runtime.deserialize_cuda_engine(f.read())
            
        self.context = self.engine.create_execution_context()
        
        # Determine dimensions of input and output bindings
        self.input_binding = self.engine[0]  # Assuming first binding is input
        self.output_binding = self.engine[1]  # Assuming second binding is output
        
        self.input_shape = self.context.get_binding_shape(0)
        self.output_shape = self.context.get_binding_shape(1)
        
        # Allocate device memory
        self.d_input = cuda.mem_alloc(np.zeros(self.input_shape, dtype=np.float32).nbytes)
        self.d_output = cuda.mem_alloc(np.zeros(self.output_shape, dtype=np.float32).nbytes)
        
        # Create CUDA stream
        self.stream = cuda.Stream()

    def infer(self, input_data):
        """
        Run inference on input data
        Args:
            input_data: numpy array matching input shape
        Returns:
            output: numpy array containing inference results
        """
        if input_data.shape != self.input_shape:
            raise ValueError(f"Input shape {input_data.shape} doesn't match engine input shape {self.input_shape}")
        
        # Transfer input data to GPU
        cuda.memcpy_htod_async(self.d_input, input_data.astype(np.float32), self.stream)
        
        # Run inference
        self.context.execute_async_v2(
            bindings=[int(self.d_input), int(self.d_output)],
            stream_handle=self.stream.handle
        )
        
        # Transfer predictions back to CPU
        output = np.zeros(self.output_shape, dtype=np.float32)
        cuda.memcpy_dtoh_async(output, self.d_output, self.stream)
        
        # Synchronize stream
        self.stream.synchronize()
        
        return output
    
    def __del__(self):
        """Cleanup when object is destroyed"""
        del self.stream
        del self.context
        del self.engine
        del self.runtime

# Example usage
def main():
    # Initialize inference engine
    engine_path = "/home/nvidia/Downloads/note-w-false-640-640-yolov5s.engine"
    trt_inference = TensorRTInference(engine_path)
    
    # Create sample input (modify shape according to your model)
    sample_input = np.random.randn(*trt_inference.input_shape).astype(np.float32)
    
    # Run inference
    output = trt_inference.infer(sample_input)
    print(f"Output shape: {output.shape}")
    print(f"Output preview: {output[:5]}")

if __name__ == "__main__":
    main()


