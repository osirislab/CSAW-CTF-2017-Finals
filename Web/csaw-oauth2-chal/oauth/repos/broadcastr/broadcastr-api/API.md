# API Endpoints
---
All endpoints are on [{{ STATUS }}]({{ STATUS }})
## GET `/api/user/info`
Requires `user:info:read` scope

Returns JSON

- `username` - User's name
- `email` - User's email

## POST `/api/user/info/update`
Requires `user:info:write` scope

Accepts JSON

- `email` - The email to change to

Returns JSON

- `username` - User's name
- `email` - User's email

## GET `/api/user/status`
Requires `user:status:read` scope

Returns JSON

- `status` - The user's last broadcasted status

## GET `/api/user/history`
Requires `user:history:read` scope

Returns JSON

- `history` - All of the user's broadcasts
