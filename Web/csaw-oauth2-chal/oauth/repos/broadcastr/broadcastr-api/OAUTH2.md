# OAuth2 Information
---
## Creating Applications
Visit the [Application Creation Page]({{ STATUS }}/app) to begin creating your application. You must be logged into an allowed user account first.

Here is more information about the options on that page:

- **Application Name** - The name of your application. This is how users know what application they are giving access.
- **Description** - Here you can provide more information about your application to the user.
- **Redirect URL** - This is where the users will be sent after authenticating. Review the OAuth2 specs for futher information.
- **Application Permissions** - Here are the OAuth2 Scopes. Normal users are only allowed to create applications with `user:info:read` and `user:status:read`. See Scopes below for more info.

Once you create the application, you will be given your OAuth2 consumer key. Keep this token safe.

## OAuth2 Setup
Once you have your consumer key, you will be able to make requests to our api. You can use standard OAuth2 calls with your key.

Here are our Oauth2 URLs:

- **Base URL** - `{{ STATUS }}/api`
- **Access Token URL** - `{{ STATUS }}/oauth/token` (using POST)
- **Authorize URL** - `{{ STATUS }}/oauth/authorize`
- **Request Token URL** - None

## OAuth2 Scopes
We provide a few scopes for applications. For more info on scopes, see the OAuth2 spec.

- `user:info:read` - Read a user's profile information, include username and email
- `user:info:write` - Change a user's email address in profile settings
- `user:status:read` - Get the most recent broadcasted status of a user
- `user:history:read` - Get all broadcasts from a user
