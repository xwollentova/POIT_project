FROM python:3.10-slim

WORKDIR /app
COPY . .

# Inštalácia systémových knižníc a Python balíčkov
RUN apt-get update && apt-get install -y \
    gcc \
    libmariadb-dev \
    && pip install --no-cache-dir -r requirements.txt

CMD ["python", "app.py"]
