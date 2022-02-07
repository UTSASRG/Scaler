

let scalerConfig;

if (process.env.NODE_ENV === "production") {
  scalerConfig = {
    $ANALYZER_SERVER_URL: "http://localhost:8081",
  };
} else {
  scalerConfig = {
    $ANALYZER_SERVER_URL: "http://localhost:8081",
  };
}

export { scalerConfig }
