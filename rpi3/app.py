from flask import Flask, jsonify, render_template

from db import get_dashboard_state, init_db


def create_app():
    app = Flask(__name__)
    app.config["SEND_FILE_MAX_AGE_DEFAULT"] = 0
    app.config["TEMPLATES_AUTO_RELOAD"] = True

    init_db()

    @app.route("/")
    def index():
        return render_template("index.html")

    @app.route("/api/state")
    def api_state():
        return jsonify(get_dashboard_state())

    return app


app = create_app()


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)