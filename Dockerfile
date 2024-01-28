FROM python:3.11-alpine

RUN apk add --no-cache curl
RUN curl -sSL https://install.python-poetry.org | python -
ENV PATH="/root/.local/bin:${PATH}"
COPY pyproject.toml poetry.lock /app/
WORKDIR /app
RUN poetry install --only=main
COPY core_metrics_aggregator/ /app/core_metrics_aggregator/
ENV PYTHONUNBUFFERED=1
CMD ["poetry", "run", "python", "-m", "core_metrics_aggregator"]
