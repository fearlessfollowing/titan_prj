import json
import cameraConnect


def commandBoilerplate(nativeCommand, oscCommand):
    body = json.dumps({"name": nativeCommand})
    prelimResponse = cameraConnect.command(body)
    responseBody = []
    responseBody.append(oscCommand)
    responseBody.append(prelimResponse["state"])
    if prelimResponse["state"] == "exception":
        responseBody[1] = "error"
        responseBody[2] = buildError('disabledCommand', 'Camera error')
    return buildResponse(responseBody)


def buildResponse(body):
    responseTemplate = {}
    responseTemplate["name"] = body[0]
    responseTemplate["state"] = body[1]
    if body[1] == "error":
        responseTemplate["error"] = body[2]
    try:
        if body[1] == "done":
            responseTemplate["results"] = body[2]
        elif body[1] == "inProgress":
            responseTemplate["id"] = body[2]
            responseTemplate["progress"] = {"completion": body[3]}
    except IndexError:
        pass
    return json.dumps(responseTemplate)


def buildError(error, msg):
    return {"code": error, "message": msg}
