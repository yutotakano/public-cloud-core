# Comment: Image base with ran-emulator installed

# Download ubuntu from the Docker Hub
FROM python:3.11-bookworm

# Install dependencies
RUN apt-get update
RUN apt-get -y install net-tools iputils-ping nano

RUN pip3 install poetry==1.8.2

WORKDIR /app

COPY ran-emulator/ran_controller/pyproject.toml .
COPY ran-emulator/ran_controller/poetry.lock .
RUN poetry install --no-dev

COPY ran-emulator/ran_controller/static ./static
COPY ran-emulator/ran_controller/templates ./templates
COPY ran-emulator/ran_controller/ran_controller.py .
COPY ran-emulator/ran_controller/eNB.py .
COPY ran-emulator/ran_controller/Status.py .
COPY ran-emulator/ran_controller/UE.py .
COPY ran-emulator/ran_controller/UserInput.py .

ENTRYPOINT [ "poetry", "run", "python" ]

CMD [ "ran_controller.py" ]
