import os, json, base64
from github import Github

label_prefix = os.environ['LABEL_PREFIX']
github_token = os.environ['GITHUB_TOKEN']
context = json.loads(os.environ['GITHUB_CONTEXT'])

pr = Github(context['token']).get_repo(context['repository']).get_pull(context['event']['number'])
for label in pr.get_labels():
    if label.name.startswith(label_prefix):
        pr.remove_from_labels(label.name)
