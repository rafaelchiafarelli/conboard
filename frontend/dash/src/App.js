
import TodoList from './TodoList'
import { useState, useRef } from 'react';
import uuidv4 from 'uuidv4'
function App() {
  const [todos, setTodos] =useState([
    {id:1,name:'Todo 1',complete:true},
    {id:2,name:'Todo 2',complete:false},
    {id:3,name:'Todo 3',complete:true},
    {id:4,name:'Todo 4',complete:false}])

  const todoNameRef = useRef()

  

  function handleAddTodo(e){
    const name = todoNameRef.current.value
    if (name === '') return
    console.log(name)
    setTodos(prevTodos => {
      return [...prevTodos, {id:uuidv4, name:name ,complete:false}]
    })
    todoNameRef.current.value = null
  }

  return (
    <>
    <TodoList todos={todos} />
    <input ref={todoNameRef} type="text" />
    <button onClick={handleAddTodo}> Add </button>
    <button> clear</button>
    <div> 0 left to do</div>
    </>
  )
}

export default App;
