FROM python:3.12-slim

WORKDIR /app

# No third-party deps currently required.
COPY validate_configs.py /app/validate_configs.py

ENTRYPOINT ["python", "/app/validate_configs.py"]
