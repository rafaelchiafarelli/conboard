import * as React from "react";
import { Admin, Resource } from "react-admin";
import jsonServerProvider from "ra-data-json-server";
import { UserList } from "./components/table/User.jsx";
import { PostList } from "./components/table/Post.jsx";
import { PostEdit } from "./components/table/EditPost.jsx";
import { PostCreate } from "./components/table/CreatePost.jsx";
import { Dashboard } from "./components/dashboard/Dashboard.jsx";

const dataProvider = jsonServerProvider("https://jsonplaceholder.typicode.com");

const App = () => (
  <Admin dashboard={Dashboard} dataProvider={dataProvider}>
    <Resource name="users" list={UserList} />
    <Resource
      name="posts"
      list={PostList}
      edit={PostEdit}
      create={PostCreate}
    />
  </Admin>
);

export default App;