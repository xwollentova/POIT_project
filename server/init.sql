CREATE TABLE IF NOT EXISTS records (
  id INT AUTO_INCREMENT PRIMARY KEY,
  temperature FLOAT NOT NULL,
  humidity FLOAT NOT NULL,
  timestamp DOUBLE NOT NULL
);
