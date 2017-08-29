# remote-build-server
DO NOT USE, ITS NOT SAFE.

## Example config

```JSON
{
	"config": {
		"projects": [
			{
				"id": "list-disk",
				"rootDir": "/home/debian/projects/list-disk",
				"type": 0
			}
		],
		"user": "admin",
		"password": "admin"
	}
}
```

## Build Types
- 0 = bash
- 1 = batch
