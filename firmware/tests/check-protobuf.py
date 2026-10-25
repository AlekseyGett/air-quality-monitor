"""Independent decoder against the actual shared schema (verification tools only)."""
import importlib.util
import pathlib
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="aqm-decoder-") as output:
    subprocess.run([sys.executable, "-m", "grpc_tools.protoc", "-I../protocol",
                    "--python_out=" + output, "../protocol/measurements.proto"], check=True)
    spec = importlib.util.spec_from_file_location("measurements_pb2", pathlib.Path(output) / "measurements_pb2.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    message = module.Measurements.FromString(pathlib.Path(sys.argv[1]).read_bytes())
    assert message.HasField("temperature") and abs(message.temperature - 23.4) < 0.0001
    assert message.HasField("humidity") and abs(message.humidity - 45.6) < 0.0001
    assert message.HasField("pressure") and message.pressure == 1013
    assert module.Measurements.FromString(b"").ListFields() == []
    def decode(suffix):
        return module.Measurements.FromString(pathlib.Path(sys.argv[1] + suffix).read_bytes())
    assert decode(".rounded").pressure == 1014
    partial = decode(".partial")
    assert partial.temperature == -12.25 and not partial.HasField("humidity") and partial.HasField("pressure")
    zero = decode(".zero")
    assert zero.HasField("temperature") and zero.temperature == 0 and not zero.HasField("pressure")
    assert decode(".empty").ListFields() == []
    assert decode(".intmax").pressure == 2147483647
    print("Independent protobuf decoding passed")
